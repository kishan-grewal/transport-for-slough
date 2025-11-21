
## https://techforum.tfl.gov.uk/t/stop-structure-station-and-stop-topology-api/2796
## https://content.tfl.gov.uk/example-api-requests.pdf

import requests
from xml.etree import ElementTree
import pandas as pd
import networkx as nx
import matplotlib.pyplot as plt

def check_is_platform(element : ElementTree.Element):
  # Check the item serves a line before the next check
  try:
    next(element.iter('itdServingLine'))
  except:
    return False

  for item in element.iter('itdServingLine'):
    name_field = item.find("./itdOperator/name")
    if name_field is not None and name_field.text == "London Underground":
      break
  else:
    return False
  return True

def create_dataframe(data : list[dict]):
  df = pd.DataFrame(data)
  df['area'] = df['area'].astype(int); df['stopID'] = df['stopID'].astype(int)
  df = df.set_index(["stopID","area"]).sort_index(level=["stopID","area"])
  return df

def get_station_ids(line: str) -> list[tuple[str, str]]:
  url = f"https://api.tfl.gov.uk/line/{line.lower()}/stoppoints"

  response = requests.get(url).json()
  return [(str(s["commonName"]),str(s["naptanId"])) for s in response]

def cast_cols_to_type(df : pd.DataFrame, cols: list[str], type : type):
  for col in cols:
    df[col] = df[col].astype(type)

station_ids = get_station_ids("jubilee")
# print("Station NAPTAN ids:")
# print(*station_ids,sep="\n")
# print()

stop = station_ids[2]
print(f"{stop[0]} information ({stop[1]})\n")
url = f"https://api.tfl.gov.uk/jp_public/api10/XML_STOPSTRUCTURE_REQUEST?sSStopNr={stop[1]}&sSOnlyDF=1&sSInclSL=1&coordOutputFormat=WGS84[dd.ddddd]&app_key=78cbd517b8b34753a87ade64492c8699"

response = requests.get(url)
tree = ElementTree.fromstring(response.content)

# print(tree.attrib)
platforms_list = []
nodes_list = []

for item in tree.iter('stopAreaLines'):
  stop_area = item.find("./stopArea"); itdPoint = item.find('./stopArea/itdPoint')
  data = (stop_area.attrib if stop_area is not None else {}) | (itdPoint.attrib if itdPoint is not None else {})

  if(check_is_platform(item)):
    platforms_list.append(data)
  nodes_list.append(data)
  
nodes = create_dataframe(nodes_list); platforms = create_dataframe(platforms_list)
cast_cols_to_type(nodes, ["x","y","level"], float)
cast_cols_to_type(platforms, ["x","y","level"], float)
del nodes_list, platforms_list

print("Nodes:\n",nodes)
# print("\n\nPlatforms:\n",platforms)

edges_list = []
for item in tree.iter('footpathInfo'):
  edge = {}
  points = item.findall("./itdPoint")
  if len(points) != 2:
    raise Exception(f"Invalid footpath - Must contain 2 points, not {len(points)}")
  edge["start"] = (int(points[0].attrib["stopID"]),int(points[0].attrib["area"]))
  edge["end"] = (int(points[1].attrib["stopID"]),int(points[1].attrib["area"]))
  
  info = item.findall("./footpathPartInfos/footpathPartInfo")
  if len(info) != 1:
    print("Warn - skipping path with multiple info fields")
    continue
  edge["type"] = info[0].attrib["type"]
  edges_list.append(edge)


edges = pd.DataFrame(edges_list)
del edges_list

fig = plt.figure()
ax = plt.axes(projection="3d")

tmp = nodes[(~nodes.isin(platforms)["level"]).values] # Select non-platform nodes
ax.scatter(tmp["x"],tmp["y"],tmp["level"])
ax.scatter(platforms["x"],platforms["y"],platforms["level"] * 0.5,c="red")
ax.legend(["Nodes","Platforms"])
del tmp

for _, edge in edges.iterrows():
  ax.plot([nodes.loc[edge["start"]]["x"],nodes.loc[edge["end"]]["x"]],
          [nodes.loc[edge["start"]]["y"],nodes.loc[edge["end"]]["y"]],
          [nodes.loc[edge["start"]]["level"]*0.5,nodes.loc[edge["end"]]["level"]*0.5],c="black",linewidth=1)


# Connect event to maintain locked roll
def on_mouse_move(event):
    # Get current view
    elev, azim = ax.elev, ax.azim # type: ignore
    # Force roll lock by resetting elev/azim only
    ax.view_init(elev=elev, azim=azim) # type: ignore
    fig.canvas.draw_idle()
fig.canvas.mpl_connect('motion_notify_event', on_mouse_move)

plt.show()

g = nx.MultiDiGraph()
for ind in nodes.index:
  g.add_node(ind, attr=nodes.loc[ind])
for _, edge in edges.iterrows():
  g.add_edge(edge["start"],edge["end"])
g.remove_nodes_from([node for node, degree in g.degree if degree == 0]) # type: ignore

fig = plt.figure(1, dpi=50)
nx.draw_spring(g,node_size=100)
plt.show()