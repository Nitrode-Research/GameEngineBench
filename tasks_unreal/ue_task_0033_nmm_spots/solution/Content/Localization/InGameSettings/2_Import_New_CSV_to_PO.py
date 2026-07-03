import os
import csv
import time

# Helper function to find the first file with a given extension in a directory
def find_file(directory, extension):
    try:
        for file in os.listdir(directory):
            if file.endswith(extension):
                return os.path.join(directory, file)
    except Exception as e:
        print(f"Error accessing directory '{directory}': {e}")
    return None

# Normalize text by removing only trailing spaces/tabs, keeping leading/middle spaces
def normalize_source(text):
    return text.rstrip(" \t") if text else ""

# Extract text from quoted CSV format
def extract_quoted_text(text):
    if text and text.startswith('"') and text.endswith('"'):
        return text[1:-1]  # Remove surrounding quotes
    return text  # Return as-is if not wrapped in quotes

# Manually find the closest match (safe, no external libraries)
def find_closest_match(target, choices):
    if not choices:
        return None  # Avoid processing if choices are empty

    best_match = None
    highest_score = 0

    for choice in choices:
        # Simple similarity check: count matching characters in order
        score = sum(1 for a, b in zip(target, choice) if a == b)

        # Prefer longer matches (more characters matching)
        if score > highest_score:
            highest_score = score
            best_match = choice

    return best_match if highest_score > len(target) * 0.6 else None  # Only suggest if 60% match

# Process each subdirectory in the current directory
directory = os.getcwd()
folder_results = {}

for sub_dir in os.listdir(directory):
    full_path = os.path.join(directory, sub_dir)
    
    if os.path.isdir(full_path) and sub_dir != "en":  # Skip 'en' (native language)
        try:
            po_file = find_file(full_path, ".po")
            csv_file = find_file(full_path, ".csv")

            if not po_file or not csv_file:
                folder_results[sub_dir] = "❌ No .po or .csv file found"
                continue

            print(f"\nProcessing folder: {sub_dir}")

            # Read CSV file and create a translation mapping
            translations = {}
            try:
                with open(csv_file, "r", encoding="utf-8") as f:
                    reader = csv.reader(f)
                    next(reader, None)  # Skip header if present
                    for row in reader:
                        if len(row) >= 2:
                            key = extract_quoted_text(row[0])  # Remove quotes
                            value = extract_quoted_text(row[1])  # Remove quotes
                            key = normalize_source(key)  # Normalize by removing trailing spaces only
                            if key and key != "None":  # Ignore "None" values
                                translations[key] = value
            except Exception as e:
                print(f"Error reading CSV file '{csv_file}': {e}")
                folder_results[sub_dir] = f"❌ CSV file error: {e}"
                continue

            # Read .po file and process msgid mapping
            try:
                with open(po_file, "r", encoding="utf-8") as f:
                    po_lines = f.readlines()
            except Exception as e:
                print(f"Error reading PO file '{po_file}': {e}")
                folder_results[sub_dir] = f"❌ PO file error: {e}"
                continue

            msgid_map = {}
            warnings = []
            po_msgid_count = 0
            log_index = 1  # Line numbering for logging

            for i in range(len(po_lines)):
                line = po_lines[i].strip()
                if line.startswith("msgid "):
                    msgid = extract_quoted_text(line[7:-1])  # Extract text
                    msgid = normalize_source(msgid)  # Normalize trailing spaces only

                    if msgid and msgid != "None":  # Ignore "None" values
                        msgid_map[msgid] = i + 1  # Store the next line index
                        po_msgid_count += 1
                        if msgid not in translations:
                            closest_match = find_closest_match(msgid, translations.keys())

                            if closest_match:
                                warning_msg = f"{log_index}. Warning: No exact match for msgid '{msgid}', did you mean: '{closest_match}'? ({sub_dir}/{os.path.basename(csv_file)})"
                            else:
                                warning_msg = f"{log_index}. Warning: No match found for msgid '{msgid}' ({sub_dir}/{os.path.basename(csv_file)})"

                            warnings.append(warning_msg)  # Append safely
                            log_index += 1

            # Apply translations to msgstr lines
            for msgid, index in msgid_map.items():
                if index < len(po_lines):
                    translation = translations.get(msgid, "")
                    po_lines[index] = f'msgstr "{translation}"\n'

            # Write the updated .po file
            try:
                with open(po_file, "w", encoding="utf-8") as f:
                    f.writelines(po_lines)
            except Exception as e:
                print(f"Error writing to PO file '{po_file}': {e}")
                folder_results[sub_dir] = f"❌ PO write error: {e}"
                continue

            print("PO file updated successfully.")

            # Compare line counts
            csv_msgid_count = len(translations)
            if po_msgid_count != csv_msgid_count:
                diff = abs(po_msgid_count - csv_msgid_count)
                mismatch_msg = f"{log_index}. Warning: PO file has {po_msgid_count} msgid entries, but CSV has {csv_msgid_count} translations. Difference: {diff} lines. ({sub_dir}/{os.path.basename(csv_file)})"
                print(mismatch_msg)
                warnings.append(mismatch_msg)

            # Store folder result
            folder_results[sub_dir] = "❌ Issues found" if warnings else "✅ Success"

        except Exception as e:
            print(f"Unexpected error in folder '{sub_dir}': {e}")
            folder_results[sub_dir] = f"❌ Fatal error: {e}"

# Display final summary
print("\n====== Translation Processing Summary ======")
for folder, status in folder_results.items():
    print(f"{folder}: {status}")

# Pause if warnings exist, otherwise close automatically
if any("❌" in status for status in folder_results.values()):
    input("\nProcess completed with warnings/errors. Press Enter to exit...")
else:
    print("\n✅ All translations processed successfully! Closing in 3 seconds...")
    time.sleep(3)
