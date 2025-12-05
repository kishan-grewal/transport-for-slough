import requests
from xml.etree import ElementTree
import pandas as pd
import numpy as np
import networkx as nx

from mpl_toolkits.mplot3d.art3d import Line3DCollection

class Station_Structure:
  def __init__(self, stop_id : str, ignore_busstops : bool = True):
    url = f"https://api.tfl.gov.uk/jp_public/api10/XML_STOPSTRUCTURE_REQUEST?sSStopNr={stop_id}&sSOnlyDF=1&sSInclSL=1&coordOutputFormat=WGS84[dd.ddddd]&app_key=78cbd517b8b34753a87ade64492c8699"

    response = requests.get(url)
    tree = ElementTree.fromstring(response.content)

    platforms_list = []
    nodes_list = []

    ## Build nodes
    for item in tree.iter('stopAreaLines'):
      stop_area = item.find("./stopArea"); itdPoint = item.find('./stopArea/itdPoint')
      data = (stop_area.attrib if stop_area is not None else {}) | (itdPoint.attrib if itdPoint is not None else {})

      if(Station_Structure.__check_is_platform(item)):
        platforms_list.append(data)

      elif ignore_busstops and item.find("./itdServingLines/itdServingLine/itdNoTrain") is not None:
        # print("Ignored bus stop node")
        continue
      nodes_list.append(data)
      
    self.nodes = Station_Structure.__create_dataframe(nodes_list); self.platforms = Station_Structure.__create_dataframe(platforms_list)

    ## Build edges
    edges_list = []
    for item in tree.iter('footpathInfo'):
      edge = {}
      points = item.findall("./itdPoint")
      if len(points) != 2:
        raise Exception(f"Invalid footpath - Must contain 2 points, not {len(points)}")
      edge["start"] = (int(points[0].attrib["stopID"]),int(points[0].attrib["area"]))
      edge["end"] = (int(points[1].attrib["stopID"]),int(points[1].attrib["area"]))

      if not (edge["start"] in self.nodes.index and edge["end"] in self.nodes.index):
        continue
      
      info = item.findall("./footpathPartInfos/footpathPartInfo")
      if len(info) != 1:
        print("Warn - skipping path with multiple info fields")
        continue
      edge["type"] = info[0].attrib["type"]
      edge["duration"] = item.attrib["duration"]
      edge["distance"] = item.attrib["distance"]
      # edge["attributes"] = info[0].find("./attributes").attrib # type: ignore
      edges_list.append(edge)


    self.edges = pd.DataFrame(edges_list)

  def drop_disconnected(self):
    indx_copy = self.nodes.index.copy()
    for node_idx in indx_copy:
      if node_idx not in self.edges["start"].values.tolist() and node_idx not in self.edges["end"].values.tolist():
        self.nodes.drop(node_idx,axis=0,inplace=True)
        
  def drop_elevators(self):
    self.edges = self.edges[self.edges["type"] != "ELEVATOR"]

  def create_nx_graph(self) -> tuple[nx.MultiDiGraph,list]:
    g = nx.MultiDiGraph()
    colours = []
    for ind in self.nodes.index:
      g.add_node(ind, attr=self.nodes.loc[ind])
      colours.append("red" if ind in self.platforms.index else "blue")
    for _, edge in self.edges.iterrows():
      g.add_edge(edge["start"],edge["end"])
    g.remove_nodes_from([node for node, degree in g.degree if degree == 0]) # type: ignore

    return g, colours

  def get_nodes_edges_plot(self):
    tmp = self.nodes[(~self.nodes.isin(self.platforms)["level"]).values] # Select non-platform nodes
    nodes_pos = np.array([tmp["x"],tmp["y"],tmp["level"] * 0.5])
    nodes_ids = np.array(tmp.index)
    platforms_pos = np.array([self.platforms["x"],self.platforms["y"],self.platforms["level"] * 0.5])
    platforms_ids = self.platforms["areaName"].values


    start = self.nodes.loc[self.edges["start"]]
    end = self.nodes.loc[self.edges["end"]]

    edge_data = {}
    for i in self.edges.index:
      edge = self.edges.loc[i]
      # if edge["type"] == "ELEVATOR" and ignore_elevators:
      #   continue

      if (edge["start"],edge["end"]) in edge_data:
        edge_data[(edge["start"],edge["end"])].append(edge["type"])
      elif (edge["end"],edge["start"]) in edge_data:
        if edge["type"] not in edge_data[(edge["end"],edge["start"])]:
          edge_data[(edge["end"],edge["start"])].append(edge["type"])
      else:
        edge_data[(edge["start"],edge["end"])] = [edge["type"]]

    def calc_pos(start, end):
      return ((start["x"] + end["x"]) / 2, (start["y"] + end["y"]) / 2, (start["level"] + end["level"]) / 4)
    edge_labels = {calc_pos(self.nodes.loc(axis=0)[key[0]],self.nodes.loc(axis=0)[key[1]]) : value for key,value in edge_data.items()}

    # Lines
    start = np.array([start["x"].values, start["y"].values, (start["level"] * 0.5).values]).T.reshape(-1,1,3)
    end = np.array([end["x"].values, end["y"].values, (end["level"] * 0.5).values]).T.reshape(-1,1,3)
    segments = np.concatenate([start, end],axis=1)
    edge_quiver = Line3DCollection(segments, colors="k", linewidths=0.5)
    

    return nodes_pos,nodes_ids, platforms_pos,platforms_ids, edge_quiver, edge_labels

  @staticmethod
  def __check_is_platform(element : ElementTree.Element):
  # Check the item serves a line before the next check
    try:
      next(element.iter('itdServingLine'))
    except:
      return False

    for item in element.iter('itdServingLine'):
      name_field = item.find("./itdOperator/name")
      ## Second check is since sometimes it's counted as a different service (e.g. at canary wharf)
      if name_field is not None and (name_field.text == "London Underground" or name_field.text == "Elizabeth line"):
        break
    else:
      return False
    return True

  @staticmethod
  def __create_dataframe(data : list[dict]):
    df = pd.DataFrame(data)
    df['area'] = df['area'].astype(int); df['stopID'] = df['stopID'].astype(int)
    df = df.set_index(["stopID","area"]).sort_index(level=["stopID","area"])
    Station_Structure.__cast_cols_to_type(df, ["x","y","level"], float)
    return df

  @staticmethod
  def __cast_cols_to_type(df : pd.DataFrame, cols: list[str], type : type):
    for col in cols:
      df[col] = df[col].astype(type)
