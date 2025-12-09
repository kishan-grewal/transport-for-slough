
## https://techforum.tfl.gov.uk/t/stop-structure-station-and-stop-topology-api/2796
## https://content.tfl.gov.uk/example-api-requests.pdf

from station_structure import Station_Structure

import requests
import networkx as nx
import matplotlib.pyplot as plt
import pandas as pd
import pickle

import time

def get_station_ids(line: str) -> list[tuple[str, str, str]]:
  url = f"https://api.tfl.gov.uk/line/{line.lower()}/stoppoints"
  # Using naptan ID csv, since the API is not 100% correct
  naptans = pd.read_csv("data-collection/station_structure/naptan.csv")

  response = requests.get(url).json()
  keys = [' '.join(s["commonName"].split(' ')[:-2]) for s in response]
  return [(str(s["commonName"]),
           str(s["naptanId"]),
           str(naptans[naptans["commonName"].apply(lambda x : x.__contains__(key))]["naptanID"].values[0]),
           )
           for s,key in zip(response, keys)]

def load_station_data(naptan):
    station = Station_Structure(naptan)
    # station.drop_elevators()
    station.drop_disconnected()

    return station

def visualise_station(fig, station : Station_Structure, name, label_nodes: bool = False, label_edges: bool = False):
    ## Visualisation
    nodes,ids, node_colours, plats, lines, edges, edge_labels = station.get_nodes_edges_plot()

    ax = fig.add_subplot(1,3,(1,2),projection="3d")
    # Connect event to maintain locked roll
    def on_mouse_move(event):
        # Get current view
        elev, azim = ax.elev, ax.azim # type: ignore
        # Force roll lock by resetting elev/azim only
        ax.view_init(elev=elev, azim=azim) # type: ignore
        fig.canvas.draw_idle()
    fig.canvas.mpl_connect('motion_notify_event', on_mouse_move)


    ax.scatter(nodes[0,:],nodes[1,:],nodes[2,:],c=node_colours)
    for platform,id in enumerate(lines):
      ax.text(plats[0,platform],plats[1,platform],plats[2,platform]-0.1,str(id),fontsize=7) #type: ignore
    ax.legend(["Nodes","Platforms"])
    ax.add_collection3d(edges) # type: ignore

    if label_nodes:
        for node,id in enumerate(ids):
          ax.text(nodes[0,node],nodes[1,node],nodes[2,node]+0.05,str(id),fontsize=5) # type: ignore
    if label_edges:
        for pos, item in edge_labels.items():
          ax.text(pos[0], pos[1], pos[2], str(item),fontsize=5) #type: ignore

    ax.grid(False); ax.set_xticks([]); ax.set_yticks([]); ax.set_zticks([]) # type: ignore
    ax.set_title(f"{name} 3D structure")

    ax2 = fig.add_subplot(1,3,3)
    g,c = station.create_nx_graph()
    # nx.draw_networkx(g,ax=ax2,node_size=100,node_color=c,with_labels=False)
    nx.draw_spring(g,ax=ax2,node_size=100,node_color=c)
    ax2.set_title(f"Directed graph")



if __name__ == "__main__":
    station_ids = get_station_ids("jubilee")
    # print(*station_ids,sep="\n")
    # print()

    stations : dict[str, Station_Structure] = {}

    for name, station_id1, station_id2 in station_ids:
        time.sleep(0.01) # Rate limit delay

        print(f"{name} information")
        # id = "9400ZZLUBNK"
        try:
            print(f"  Trying {station_id1}\t",end="")
            stations[name] = load_station_data(station_id1)
            print()
            continue
        except Exception as e:
            print(f"  Exception [{e}]")

        if(station_id2 != station_id1):
          try:
              print(f"  Trying {station_id2}\t",end="")
              stations[name] = load_station_data(station_id2)
              print()
              continue
          except Exception as e:
              print(f"  Exception [{e}]")

        station_id3 = "9400"+station_id1[4:]
        try:
            print(f"  Trying {station_id3}\t",end="")
            stations[name] = load_station_data(station_id3)
            print()
            continue
        except Exception as e:
            print(f"  Exception [{e}]")
            
        station_id4 = "9400"+station_id2[4:]
        if station_id4 != station_id3:
            try:
                print(f"  Trying {station_id4}\t",end="")
                stations[name] = load_station_data(station_id4)
                print()
                continue
            except Exception as e:
                print(f"  Exception [{e}]")

        print("  WARN - failed to find any station data")

    for name, station in stations.items():
        fig = plt.figure()
        visualise_station(fig, station, name, label_nodes=True)
        plt.savefig(f"data-collection/station_structure/outputs/{name}.png")
        plt.close()
    # s = stations["Canada Water Underground Station"]
    # print(s.nodes)
    # print(s.edges)
    # visualise_station(s, "Canada Water Underground Station",label_nodes=True, label_edges=True)
    # plt.show()

    with open("data-collection/station_structure/outputs/stations.pkl", "wb") as f:
        pickle.dump(stations,f)