from selenium import webdriver
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.by import By
from selenium.webdriver.support.wait import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC

import uuid
import sqlite3
import re



expr  = re.compile(r"([\d.]*\d)(b|kb|mb|gb)", re.IGNORECASE)

def get_filesize(str_in):
    match = re.match(expr, str_in)
    value = 0
    try:
        modifier = 1
        if match.group(2).lower() == "kb":
            modifier = 8**3
        elif match.group(2).lower() == "mb":
            modifier = 8**6
        elif match.group(2).lower() == "gb":
            modifier = 8**9
        value = int(float(match.group(1)) * modifier)
        return value
    except:
        return value


con = sqlite3.connect("datasets.db")
cur = con.cursor()
cur.execute("CREATE TABLE IF NOT EXISTS sources(source_UID TEXT, properties TEXT, domain TEXT, subdomain TEXT, citation TEXT)")
cur.execute("CREATE TABLE IF NOT EXISTS datasets(dataset_UID TEXT, source_UID TEXT, name TEXT, node_count INT, edge_count INT, file_size INT, file_format TEXT, data_format TEXT, url TEXT)")


driver = webdriver.Firefox()
driver.get("https://icon.colorado.edu/#!/networks")
assert "Index of Complex Networks" in driver.title

"""
elem = driver.find_element(By.NAME, "q")
elem.clear()
elem.send_keys("pycon")
elem.send_keys(Keys.RETURN)
assert "No results found." not in driver.page_source
"""
try:
    element = WebDriverWait(driver, 10).until(
        EC.presence_of_element_located((By.XPATH, "/html/body/section/section/section/section/div/md-list/div[1]/div/div/network-list-item/div/md-list-item/div/div[2]/button[1]"))
    )
except selenium.common.exceptions.TimeoutException:
    driver.close()

network_expand_button_list = []

old_network_count = -1
new_network_count = 0
while new_network_count > old_network_count:
    old_network_count = new_network_count
    driver.execute_script("window.scrollTo(0, document.body.scrollHeight);")
    network_expand_button_list = driver.find_elements(By.XPATH, "//button[@ng-click=\"network.state = 'open'\"]")
    new_network_count = len(network_expand_button_list)

for button in network_expand_button_list:
    button.click()

print(len(network_expand_button_list))

networkCards = driver.find_elements(By.CLASS_NAME, "networkCard")

for card in networkCards:
    domain = card.find_element(By.CLASS_NAME, "domain").text
    subdomain = card.find_element(By.CLASS_NAME, "subDomain").text
    properties = card.find_element(By.CLASS_NAME, "graphPropertiesContent").text
    citation = card.find_element(By.CLASS_NAME, "networkSource").find_element(By.CLASS_NAME, "ng-binding").text
    #print(citation)
    print(f"Properties: {properties}, Domain: {domain}, {subdomain}")

    source_UID = str(uuid.uuid4())
    cur.execute("INSERT INTO sources(source_UID, properties, domain, subdomain, citation) VALUES(?, ?, ?, ?, ?)", [source_UID, properties, domain, subdomain, citation])
    data_file_link_rows = card.find_element(By.CLASS_NAME, "networksContent").find_elements(By.XPATH, "./table/tbody/tr[@ng-repeat=\"graph in network._source.graphs\"]")
    for data_file_link_row in data_file_link_rows:
        dataset_UID = str(uuid.uuid4())
        name = data_file_link_row.find_element(By.XPATH, "./td[1]").text
        node_count = data_file_link_row.find_element(By.XPATH, "./td[2]").text
        edge_count = data_file_link_row.find_element(By.XPATH, "./td[3]").text

        filesize_str = data_file_link_row.find_element(By.XPATH, "./td[4]").text
        filesize = get_filesize(filesize_str)

        file_format = data_file_link_row.find_element(By.XPATH, "./td[5]").text
        data_format = data_file_link_row.find_element(By.XPATH, "./td[6]").text
        url = data_file_link_row.find_element(By.XPATH, "./td[7]/a").get_attribute("href")
        print(f"\tName: {name}, Nodes: {node_count}, Edges: {edge_count}, File Size: {filesize}, File Format: {file_format}, Data Format: {data_format}, Link: {url}")
        cur.execute("INSERT INTO datasets(dataset_UID, source_UID, name, node_count, edge_count, file_size, file_format, data_format, url) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)", [dataset_UID, source_UID, name, node_count, edge_count, filesize, file_format, data_format, url])
    con.commit()

con.close()
driver.close()