import selenium
from selenium import webdriver
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.by import By
from selenium.webdriver.support.wait import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC

import sqlite3
import os
import subprocess
import tarfile


con = sqlite3.connect("datasets_konect.db")
cur = con.cursor()
cur.execute("CREATE TABLE IF NOT EXISTS datasets(internal_name TEXT PRIMARY KEY, name TEXT, source TEXT, node_count INT, edge_count INT, category TEXT, network_format TEXT, edge_type TEXT, link TEXT)")

firefox_options = webdriver.FirefoxOptions()
firefox_options.set_preference('permissions.default.image', 2)
firefox_options.set_preference('dom.ipc.plugins.enabled.libflashplayer.so', 'false')

driver = webdriver.Firefox(firefox_options)

driver.get("http://www.konect.cc/networks/")
assert "Networks" in driver.title

link_eles = driver.find_elements(By.XPATH, "//td[2]/a")

links = []
for link_ele in link_eles:
    link = link_ele.get_attribute("href")
    links.append(link)

for link in links:
    try:
        driver.get(link)
        element = WebDriverWait(driver, 2).until(
            EC.presence_of_element_located((By.XPATH, "//a"))
        )
    except selenium.common.exceptions.TimeoutException:
        print(f"Timeout Exception!")
        continue
    internal_name = driver.find_element(By.XPATH, "/html/body/div[2]/table[1]/tbody/tr[2]/td[3]/code").text
    name = driver.find_element(By.XPATH, "/html/body/div[2]/table[1]/tbody/tr[3]/td[3]").text
    category = driver.find_element(By.XPATH, "//a[contains(text(), 'Category')]/../../td[3]/a").text
    network_format = driver.find_element(By.XPATH, "//td[contains(text(), 'Network format')]/../td[3]").text
    edge_type = driver.find_element(By.XPATH, "//td[contains(text(), 'Edge type')]/../td[3]").text
    node_count = int(driver.find_element(By.XPATH, "//a[contains(text(), 'Size')]/../../td[3]").text.replace(",",""))
    edge_count = int(driver.find_element(By.XPATH, "//a[contains(text(), 'Volume')]/../../td[3]").text.replace(",",""))
    download_url = ""
    try:
        download_url = driver.find_element(By.XPATH, "//a[contains(text(), 'Dataset is available for download')]").get_attribute("href")
    except selenium.common.exceptions.NoSuchElementException:
        print(f"{name}[{internal_name}] not availabile for download.")
        continue

    cur.execute("INSERT OR REPLACE INTO datasets(internal_name, name, node_count, edge_count, category, network_format, edge_type, link) VALUES(?, ?, ?, ?, ?, ?, ?, ?)", [internal_name, name, node_count, edge_count, category, network_format, edge_type, download_url])
    con.commit()

driver.close()