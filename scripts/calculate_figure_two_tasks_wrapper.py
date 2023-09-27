# Importing required module
import os, sqlite3, subprocess, tarfile,shutil
from pathlib import Path

con = sqlite3.connect("tasks.db")
cur = con.cursor()

"""
TASK_ID URL WEIGHTED DIRECTED COMPLETED
"""

while cur.execute("SELECT COUNT(TASK_ID) FROM tasks WHERE COMPLETED = 0").fetchone()[0] > 0:
    task = cur.execute("SELECT TASK_ID, URL, WEIGHTED,DIRECTED FROM tasks WHERE COMPLETED = 0").fetchone()
    task_id = task[0]
    url = task[1]
    weighted = task[2]
    directed = task[3]

    path = os.path.join(os.getcwd(),f'datasets/Konect/{task_id}')
    try:
        os.mkdir(path)
    except:
        pass
    filepath = os.path.join(path, 'data.tar.bz2')
    subprocess.run(["wget", "-O", filepath, "-q", url])

    tar = None
    try:
        tar = tarfile.open(filepath)
    except tarfile.ReadError:
        print(f"Error opening {task_id}'s tar file at {filepath}!")
        continue
    contents = tar.getnames()
    folder = contents.pop(0)
    for name in contents:
        if "/out." in name:
            tar.extract(name, path=path)
    output_path = os.path.join(path, folder)
    op = Path(output_path)
    if not os.path.isdir(output_path):
        print(f"Error unpacking {task_id}'s tar file!")
        continue
    datapath = ""
    for item in op.iterdir():
        if "out." in item.as_posix():
            datapath = item
    if not os.path.isfile(datapath):
        print(f"Error locating {task_id}'s unpacked data file!")
        continue

    os.system(f"./cpp/figure_2_s_avg_calculator.bin {task_id} {datapath} {weighted} {directed}")

    cur.execute("UPDATE tasks SET COMPLETED = 1 WHERE TASK_ID = ?", [task_id])
    con.commit()
    shutil.rmtree(path)