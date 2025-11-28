
## https://techforum.tfl.gov.uk/t/stop-structure-station-and-stop-topology-api/2796
## https://content.tfl.gov.uk/example-api-requests.pdf

from station_structure import Station_Structure

import requests
import networkx as nx
import matplotlib.pyplot as plt


def get_station_ids(line: str) -> list[tuple[str, str]]:
  url = f"https://api.tfl.gov.uk/line/{line.lower()}/stoppoints"

  response = requests.get(url).json()
  return [(str(s["commonName"]),str(s["naptanId"])) for s in response]

station_ids = get_station_ids("jubilee")
# print("Station NAPTAN ids:")
# print(*station_ids,sep="\n")
# print()

stop = station_ids[9]
print(f"{stop[0]} information ({stop[1]})\n")
station = Station_Structure(stop[1])
# station = Station_Structure("9400ZZLUBNK")
station.drop_disconnected()

## Visualisation
nodes,ids, plats, lines, edges, edge_labels = station.get_nodes_edges_plot()

fig = plt.figure()

ax = fig.add_subplot(1,3,(1,2),projection="3d")
# Connect event to maintain locked roll
def on_mouse_move(event):
    # Get current view
    elev, azim = ax.elev, ax.azim # type: ignore
    # Force roll lock by resetting elev/azim only
    ax.view_init(elev=elev, azim=azim) # type: ignore
    fig.canvas.draw_idle()
fig.canvas.mpl_connect('motion_notify_event', on_mouse_move)


ax.scatter(nodes[0,:],nodes[1,:],nodes[2,:])
ax.scatter(plats[0,:],plats[1,:],plats[2,:],c="red")
# for node,id in enumerate(ids):
#    ax.text(nodes[0,node],nodes[1,node],nodes[2,node]+0.05,str(id),fontsize=5) # type: ignore
for platform,id in enumerate(lines):
   ax.text(plats[0,platform],plats[1,platform],plats[2,platform]-0.1,str(id),fontsize=7) #type: ignore
ax.legend(["Nodes","Platforms"])
ax.add_collection3d(edges) # type: ignore
for pos, item in edge_labels.items():
   ax.text(pos[0], pos[1], pos[2], str(item),fontsize=5) #type: ignore

ax.grid(False); ax.set_xticks([]); ax.set_yticks([]); ax.set_zticks([]) # type: ignore

ax2 = fig.add_subplot(1,3,3)
g,c = station.create_nx_graph()
nx.draw_spring(g,ax=ax2,node_size=100,node_color=c)
plt.show()