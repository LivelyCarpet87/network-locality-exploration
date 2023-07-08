from selenium import webdriver
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.by import By
from selenium.webdriver.support.wait import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import selenium

import sqlite3, os, requests, magic, subprocess, zipfile, gzip, bz2

from tqdm import tqdm

import format as f

success = 0
attempts = 0

def process_file(filepath, nodes, edges, filesize):
    global success, attempts
    if not os.path.isfile(filepath):
        print("Error: File failed to download")
        return None
    """
    if os.path.getsize(filepath) not in range(int(filesize*0.9), int(filesize*1.1)):
        print(f"Filesize match fail {os.path.getsize(filepath)} VS {filesize}")
        return None
    """
    filetype = magic.from_file(filepath)
    if filetype.startswith("ASCII text") or filetype.startswith("CSV") or filetype.startswith("Unicode text") or filetype.startswith("C source"):
        #print(f"Need TXT Processing")
        df = f.parse_via_regex(filepath, r"(\d+)[ \t,]+(\d+)[ \t,]+(\d+(?:\.\d+(?:[Ee]-\d+)?)?)")
        if df is not None and df.shape[0] in range(edges-1, int(edges+2)):
            success += 1
            print(f"File Found {success}/{attempts}")
            return df
        else:
            return None
    elif filetype.startswith("HTML document"):
        return None
    elif filetype.startswith("Zip archive data"):
        #print(f"Need Zip Processing")
        zipfilehandle = zipfile.ZipFile(filepath)
        for name in zipfilehandle.namelist():
            dir = filepath[:-4]
            filepath_zip = os.path.join(dir,name)
            zipfilehandle.extract(name, path=dir)
            df = process_file(filepath_zip, nodes, edges, filesize)
            #os.remove(filepath_zip)
            if df is not None and df.shape[0] in range(edges-1, int(edges+2)):
                success += 1
                print(f"File Found {success}/{attempts}")
                return df
            else:
                return None
        return None
    elif filetype.startswith("gzip compressed data"):
        # print(f"Need Gzip Processing")
        # content = gzip.open(filepath, mode='r').read()
        # df = f.parse_content_via_regex(content, r"(\d+)[ \t,]+(\d+)[ \t,]+(\d+(?:\.\d+(?:[Ee]-\d+)?)?)")
        # if df is not None and df.shape[0] in range(edges-1, int(edges+2)):
        #     print(f"File Found")
        #     return df
        # else:
        #     return None
        return None
    elif filetype.startswith("bzip2 compressed data"):
        # print(f"Need Gzip Processing")
        # content = bz2.open(filepath, mode='r').read()
        # df = f.parse_content_via_regex(content, r"(\d+)[ \t,]+(\d+)[ \t,]+(\d+(?:\.\d+(?:[Ee]-\d+)?)?)")
        # if df is not None and df.shape[0] in range(edges-1, int(edges+2)):
        #     print(f"File Found")
        #     return df
        # else:
        #     return None
        return None
    elif filetype.startswith("Microsoft Excel"):
        print(f"Need Excel Processing")
        return None
    elif filetype.startswith("XML"):
        #print(f"Need XML Processing")
        return None
    elif filetype.startswith("empty"):
        return None
    else:
        #print(f"Error: Unrecognized file {filetype}")
        return None

def process_link(filepath, link, nodes, edges, filesize):
    if filepath is None:
        print("Error: Path is None")
    if link is None:
        print("Error: Link is None")
    try:
        subprocess.run(["wget", "-O", str(filepath), "-q", "--timeout=30", str(link)], timeout=35)
    except subprocess.TimeoutExpired:
        print("Error: Download Timed Out\n")
        return None
    df_res = process_file(filepath, nodes, edges, filesize)
    if df_res is None:
        os.remove(filepath)
        return None
    else:
        return df_res



con = sqlite3.connect("datasets.db")
cur = con.cursor()

driver = webdriver.Firefox()

res1 = cur.execute("SELECT source_UID FROM sources")
source_UIDS = res1.fetchall()

for source_UID_res in source_UIDS:
    source_UID = source_UID_res[0]
    try:
        os.mkdir(f"./datasets/ICON/{source_UID}")
    except FileExistsError:
        pass

    res2 = cur.execute("SELECT dataset_UID, url, node_count, edge_count, file_size FROM datasets WHERE source_UID = ? and file_format = 'txt'", [source_UID])
    dataset_entries = res2.fetchall()
    
    for dataset_entry in dataset_entries:
        dataset_UID = dataset_entry[0]
        url = dataset_entry[1]
        node_count = dataset_entry[2]
        edge_count = dataset_entry[3]
        filesize = dataset_entry[4]
        attempts += 1
        try:
            os.mkdir(f"./datasets/ICON/{source_UID}/{dataset_UID}")
        except FileExistsError:
            pass
        try:
            if "html" not in requests.head(url, allow_redirects=True).headers.get("content-type", ''):
                filepath = os.path.join(os.getcwd(),f'datasets/ICON/{source_UID}/{dataset_UID}/tmp')
                process_link(filepath, url, node_count, edge_count, filesize)
                continue
        except requests.exceptions.ConnectTimeout:
            print(f"Error: {source_UID}/{dataset_UID} Webpage Connect Timeout")
            continue
        except requests.exceptions.ReadTimeout:
            print(f"Error: {source_UID}/{dataset_UID} Webpage Read Timeout")
            continue
        except requests.exceptions.SSLError:
                continue

        try:
            driver.get(url)
            element = WebDriverWait(driver, 2).until(
                EC.presence_of_element_located((By.XPATH, "//a"))
            )
        except selenium.common.exceptions.TimeoutException:
            print(f"{source_UID}/{dataset_UID} Timeout Exception!")
            continue
        link_eles = driver.find_elements(By.XPATH, "//a")
        driver.save_screenshot(os.path.join(os.getcwd(),f'datasets/ICON/{source_UID}/{dataset_UID}/screenshot.png'))
        df_res = None
        for link_ele in link_eles:
            link = ""
            try:
                link = link_ele.get_attribute("href")
                #print(link)
            except selenium.common.exceptions.StaleElementReferenceException:
                continue
            except requests.exceptions.SSLError:
                continue
            if link is None:
                print(f"{source_UID}/{dataset_UID} No valid link found in href!")
                continue
            elif link.endswith(".pdf") or link.endswith(".html") or link.endswith(".php") or link.endswith(".cfm") or link.endswith(".png") or link.endswith(".jpg") or link.endswith(".htm") or link.endswith(".jsp") or link.startswith("mailto:") or link.startswith("javascript:"):
                continue
            elif "." not in link.split("/")[-1] or "#" in link.split("/")[-1]:
                continue
            filepath = os.path.join(os.getcwd(),f'datasets/ICON/{source_UID}/{dataset_UID}/tmp')
            df_res = process_link(filepath, link, node_count, edge_count, filesize)
            if df_res is not None:
                break
        if df_res is None:
            print(f"{source_UID}/{dataset_UID} Error: No valid file located")

