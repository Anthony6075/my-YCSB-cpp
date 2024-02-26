import csv
import sys

column_width = 15

inputfile_name = sys.argv[1]
outputfile_name = inputfile_name + '.tsv'

data = {}

db_names = []
workload_names = []

with open(inputfile_name) as inputfile:
    for line in inputfile:
        line = line.strip()
        tokens = []
        for t in line.split(' '):
            t = t.strip()
            if len(t) != 0:
                tokens.append(t)

        workload = tokens[0]
        db = tokens[1]
        load_tp = tokens[2]
        run_tp = tokens[3]

        if workload not in workload_names:
            workload_names.append(workload)

        if db not in db_names:
            db_names.append(db)

        if workload not in data:
            data[workload] = {}

        data[workload][db] = load_tp + 'k/' + run_tp + 'k'

workload_names.sort()
db_names.sort()

workload_names[:] = [wl.ljust(column_width) for wl in workload_names]
db_names[:] = [db.ljust(column_width) for db in db_names]

def get_data(all, wl, db):
    wl = wl.strip()
    db = db.strip()
    if wl not in all:
        return "###"
    if db not in all[wl]:
        return "###"
    return all[wl][db]

with open(outputfile_name, 'w', newline='') as output_file:
    writer = csv.writer(output_file, delimiter='\t', lineterminator='\n')
    writer.writerow(['workload'.ljust(column_width)] + db_names)
    
    for wl in workload_names:
        row = [wl]
        for db in db_names:
            row.append(get_data(data, wl, db).ljust(column_width))
        writer.writerow(row)
    
