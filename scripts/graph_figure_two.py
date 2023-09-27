import matplotlib.pyplot as plt
import sqlite3
import numpy as np

con = sqlite3.connect("result.db")
cur = con.cursor()

points = cur.execute("""
SELECT d.node_count, s.avg_s, d.category 
FROM S_average s
INNER JOIN datasets d ON
s.NET_ID = d.internal_name;
""").fetchall()

social = []
computer = []
biological = []
infrastructure = []
authorship = []
ws = []

for point in points:
    x = point[0]
    y = point[1]
    category = point[2].lower()
    if "social" in category or "communication" in category or "contact" in category:
        social.append( [x,y] )
    elif "hyperlink" in category or "computer" in category:
        computer.append( [x,y] )
    elif "infrastructure" in category:
        infrastructure.append( [x,y] )
    elif "citation" in category or "author" in category:
        authorship.append( [x,y] )
    elif "metabolic" in category or "biological" in category:
        biological.append( [x,y] )

# Instance plot
fig, ax = plt.subplots()

# Configure X axis
ax.set_xscale("log")
ax.set_xlim(left=1E3,right=1E5)

# Configure Y axis
ax.set_ylim(bottom=0,top=30)

# Plot guide curves
X = np.linspace(1E3, 1E5, int(1E4))

l_gamma_1 = 1E-2 * X
l_gamma_2 = 5E-3 * X
l_gamma_3 = 2E-3 * X
l_gamma_4 = 1E-3 * X
l_gamma_5 = 5E-4 * X
l_gamma_6 = 2E-4 * X
l_gamma_7 = 1E-4 * X

ax.plot(X,l_gamma_1, c="0.8", linestyle="--")
ax.plot(X,l_gamma_2, c="0.8", linestyle="--")
ax.plot(X,l_gamma_3, c="0.8", linestyle="--")
ax.plot(X,l_gamma_4, c="0.8", linestyle="--")
ax.plot(X,l_gamma_5, c="0.8", linestyle="--")
ax.plot(X,l_gamma_6, c="0.8", linestyle="--")
ax.plot(X,l_gamma_7, c="0.8", linestyle="--")

# Plot data
ax.scatter(np.array(social)[:,0], np.array(social)[:,1], label="Social")
ax.scatter(np.array(computer)[:,0], np.array(computer)[:,1], label="Computer")
ax.scatter(np.array(biological)[:,0], np.array(biological)[:,1], label="Biological")
ax.scatter(np.array(infrastructure)[:,0], np.array(infrastructure)[:,1], label="Infrastructure")
ax.scatter(np.array(authorship)[:,0], np.array(authorship)[:,1], label="Authorship")

ax.plot([1E3, 5E3, 1E4,3E4,5E4,6E4,7E4,1E5],[9.1903604999999988, 8.7096329999999984, 8.6390954999999998,7.1698755000000007,(33.026139999999998/4), 8.3224830000000001, 2.0128840000000006, 2.07], marker='^', label="WS Model")

legend = ax.legend()
ax.set_xlabel('N')
ax.set_ylabel('avg_S(gamma)')
ax.set_title('Title')

plt.show()