import os
import csv
import time

# Helper function to find the first .po file in a directory
def find_po_file(directory):
    for file in os.listdir(directory):
        if file.endswith(".po"):
            return os.path.join(directory, file)
    return None

# Normalize text by removing only trailing spaces/tabs, keeping leading/middle spaces
def normalize_text(text):
    return text.rstrip(" \t")

# Extract quoted text from a .po file line
def extract_quoted_text(text):
    if text.startswith('"') and text.endswith('"'):
        return text[1:-1]  # Remove surrounding quotes
    return text  # Return as-is if not wrapped in quotes

# Process each subdirectory in the current directory
directory = os.getcwd()
folder_results = {}

for sub_dir in os.listdir(directory):
    full_path = os.path.join(directory, sub_dir)
    
    if os.path.isdir(full_path):  # Only process directories
        po_file = find_po_file(full_path)
        if not po_file:
            folder_results[sub_dir] = "❌ No .po file found"
            continue
        
        csv_file = os.path.join(full_path, f"{sub_dir}.csv")  # CSV file named after the folder

        # Remove existing CSV file to create a new one
        if os.path.exists(csv_file):
            os.remove(csv_file)

        print(f"\n📂 Processing: {sub_dir}")

        # Read .po file and extract msgid and msgstr pairs
        entries = []
        with open(po_file, "r", encoding="utf-8") as f:
            po_lines = f.readlines()

        msgid = None
        msgstr = None
        po_msgid_count = 0

        for line in po_lines:
            line = line.strip()

            if line.startswith("msgid "):
                msgid = extract_quoted_text(line[7:-1])
                msgid = normalize_text(msgid)

                if not msgid or msgid == "None":  # Skip empty or "None" keys
                    msgid = None
                msgstr = None  # Reset msgstr when a new msgid starts

            elif line.startswith("msgstr "):
                msgstr = extract_quoted_text(line[8:-1])
                msgstr = normalize_text(msgstr) if msgstr else ""

                if msgid is not None and msgstr != "None":  # Skip "None" translations
                    entries.append((msgid, msgstr))
                    po_msgid_count += 1
                    msgid = None  # Reset after storing

        # Write extracted data to CSV
        with open(csv_file, "w", encoding="utf-8", newline="") as f:
            writer = csv.writer(f, quotechar='"', quoting=csv.QUOTE_ALL)
            writer.writerow(["Source", "Translation"])  # Updated column names
            writer.writerows(entries)

        print(f"✅ CSV file created: {csv_file} (Total entries: {po_msgid_count})")
        folder_results[sub_dir] = "✅ Success"

# Display final summary
print("\n====== Extraction Summary ======")
for folder, status in folder_results.items():
    print(f"{folder}: {status}")

# Pause if warnings exist, otherwise close automatically
if any("❌" in status for status in folder_results.values()):
    input("\n⚠️ Process completed with warnings/errors. Press Enter to exit...")
else:
    print("\n✅ All extractions completed successfully! Closing in 3 seconds...")
    time.sleep(3)
