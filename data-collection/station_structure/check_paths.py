import json
import networkx as nx
import matplotlib.pyplot as plt

with open("station_structures.json","rb") as f:
  s = json.load(f)
assert s is not None

struc = s["Canning Town Underground Station"]["structure"]

def add_node(g, struc, id, reverse=False):
  elem = struc[id]
  
  g.add_edge(elem["prev"]//2,elem["id"]//2)
  g.add_edge(elem["id"]//2,elem["next"]//2)

  if elem["prev"] == elem["next"]:
    print(f"WARN - id {id} invalid")

  if "secondary" in elem:
    if reverse:
      g.add_edge(elem["id"]//2,elem["secondary"]//2)
    else:
      g.add_edge(elem["secondary"]//2,elem["id"]//2)

g = nx.DiGraph()
c = []
struc = list(struc)
for elem in struc:
  if "next" in elem and "id" in elem and "prev" in elem:
    if elem["adjacent"] in g.nodes:
      add_node(g,struc,elem["adjacent"],True)
    else:
      add_node(g,struc,elem["id"])

nx.draw_spring(g,with_labels=True)
plt.show()