import csv

input_file = "c:/Users/guhas/src/CAPSTONE/Running_12th/ekf_state_log_asst.csv"  # Change this to your actual input file
output_file = "output.csv"  # Change this to your desired output file

with open(input_file, "r") as infile, open(output_file, "w", newline="") as outfile:
    reader = csv.reader(infile)
    writer = csv.writer(outfile)
    
    header = next(reader)  # Read the header
    
    if len(header) == 13:  # Add new column names if missing
        header.extend(["filtered_gyro_x", "filtered_gyro_y", "filtered_gyro_z"])
    
    writer.writerow(header)  # Write updated header
    
    for row in reader:
        if len(row) == len(header) - 3:  # If extra values are on a new line
            next_row = next(reader, [])
            row.extend(next_row)  # Append the next row's values
        writer.writerow(row)
