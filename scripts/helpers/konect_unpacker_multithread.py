from pathlib import Path
import shutil
import sqlite3, os, tarfile
import subprocess
from multiprocessing.pool import ThreadPool
from tqdm import tqdm
import concurrent.futures

import format as f

def process_entry(entry):
    internal_name = entry[0]
    node_count = entry[1]
    edge_count = entry[2]
    network_format = entry[3]
    edge_type = entry[4]
    download_url = entry[5]
    
    unweighted = False
    if "unweighted" in edge_type.lower():
        unweighted = True
    directional = False
    if "directed" in network_format.lower():
        directional = True

    path = os.path.join(os.getcwd(),f'datasets/Konect/{internal_name}')
    try:
        os.mkdir(path)
    except:
        pass
    filepath = os.path.join(path, 'data.tar.bz2')
    subprocess.run(["wget", "-O", filepath, "-q", download_url])

    tar = None
    try:
        tar = tarfile.open(filepath)
    except tarfile.ReadError:
        print(f"Error opening {internal_name}'s tar file!")
        return
    contents = tar.getnames()
    folder = contents.pop(0)
    for name in contents:
        if "/out." in name:
            tar.extract(name, path=path)
        """
        elif "/meta." in name:
            tar.extract(name, path=path)
        """
    output_path = os.path.join(path, folder)
    op = Path(output_path)
    if not os.path.isdir(output_path):
        print(f"Error unpacking {internal_name}'s tar file!")
        return
    datapath = ""
    for item in op.iterdir():
        if "out." in item.as_posix():
            datapath = item
    if not os.path.isfile(datapath):
        print(f"Error locating {internal_name}'s unpacked data file!")
        return
    df = f.parse_via_regex(datapath, r"^(\d+)[ \t,]+(\d+)(?:[ \t,]+([-+]?\d?(?:\.\d+)?(?:(?:[Ee]-\d+)?)?))?", order=[0,1,2], unweighted=unweighted)
    if df.shape[0] != edge_count:
        print(f"{df.shape[0]} != {edge_count}, {internal_name} may have errors.")
        return
    G = f.create_graph(df, directional=directional)
    shutil.rmtree(path)

con = sqlite3.connect("datasets_konect.db")
cur = con.cursor()

res1 = cur.execute("SELECT internal_name, node_count, edge_count, network_format, edge_type, link FROM datasets")
entries = res1.fetchall()

with concurrent.futures.ThreadPoolExecutor(max_workers=8) as executor:
    results = list(tqdm(executor.map(process_entry, entries), total=len(entries)))