import paths
import sys
import pickle
import math
import copy

import pandas as pd
import numpy as np

import json
import networkx as nx

def vincenty_sphere_distance(lat1, lon1, lat2, lon2, radius=6371008.8):
    """
    Compute great-circle distance using the Vincenty formula for a sphere.

    Parameters:
        lat1, lon1 -- latitude/longitude of point 1 in degrees
        lat2, lon2 -- latitude/longitude of point 2 in degrees
        radius     -- sphere radius in meters (default: Earth's mean radius)

    Returns:
        Distance in meters.
    """

    # Convert degrees to radians
    lat1_rad = math.radians(lat1)
    lon1_rad = math.radians(lon1)
    lat2_rad = math.radians(lat2)
    lon2_rad = math.radians(lon2)

    dlon = lon2_rad - lon1_rad

    # Components of the Vincenty spherical formula
    num = math.sqrt(
        (math.cos(lat2_rad) * math.sin(dlon))**2 +
        (math.cos(lat1_rad) * math.sin(lat2_rad) -
         math.sin(lat1_rad) * math.cos(lat2_rad) * math.cos(dlon))**2
    )

    den = (
        math.sin(lat1_rad) * math.sin(lat2_rad) +
        math.cos(lat1_rad) * math.cos(lat2_rad) * math.cos(dlon)
    )

    # Great-circle distance
    distance = radius * math.atan2(num, den)
    return distance

SLICE_LEN = 1
PLATFORM_LEN = 120

class EdgeData:
  def __init__(self, start, end, reversible, path_type, forward_flows, reverse_flows):
    self.reversible = reversible
    self.start_node = start
    self.end_node = end

    self.forward_flows = forward_flows.copy()
    self.reverse_flows = reverse_flows.copy()
    pass

  def matches(self, start, end):
    return self.start_node.name == start and self.end_node.name == end

  def to_dict(self, idx_offset = 0):
    segments = []
    start_idx = []
    end_idx = []

    last_fwd = -1
    last_rev = -1

    if "entrance" in self.start_node["nodeType"]:
      segments.append({"type":"AREA_OUTFLOW","next":-1,"xk":1,"is_entrance":True})
      last_fwd = len(segments)-1
      start_idx.append(last_fwd + idx_offset)

      end_idx = [last_fwd]
      if self.reversible:
        segments.append({"type":"AREA_INFLOW","prev":-1,"xk":2147483647})
        last_rev = len(segments)-1
        start_idx.append(last_rev + idx_offset)
        end_idx = [*end_idx, last_rev]
    else:
      start_idx = [idx_offset]
      end_idx = [-1]
      if self.reversible:
        start_idx = [idx_offset, 1 + idx_offset]
        end_idx = [-1,-1]
    
    distance = vincenty_sphere_distance(float(self.start_node["x"]),float(self.start_node["y"]),float(self.end_node["x"]),float(self.end_node["y"]))
    n_slices = max(1,math.ceil(distance / SLICE_LEN)) # Ensure there is always one segment, even for negligible length regions
    n_slices = 1

    for _ in range(n_slices):
      if last_fwd != -1: segments[last_fwd]["next"] = len(segments) + idx_offset

      segments.append({"id":len(segments)+idx_offset,"type":"DIRECT","next":-1,"prev":last_fwd + idx_offset,"xk":SLICE_LEN})
      last_fwd = len(segments) - 1
      end_idx[0] = last_fwd + idx_offset

      if self.reversible:
        if last_rev != -1: segments[last_rev]["prev"] = len(segments) + idx_offset
        
        adj_i = len(segments)-1
        segments.append({"id":len(segments)+idx_offset,"type":"DIRECT","next":last_rev + idx_offset,"prev":-1,"xk":SLICE_LEN,"adjacent":adj_i + idx_offset})
        segments[adj_i]["adjacent"] = len(segments)-1 + idx_offset
        last_rev = len(segments) - 1
        end_idx[1] = last_rev + idx_offset

    if "platform" in self.end_node["nodeType"]:
      if last_fwd != -1: segments[last_fwd]["next"] = len(segments) + idx_offset

      platform_id = "NONE"
      splt = self.end_node["nodeType"].split(' ')
      if len(splt) > 1:
        platform_id = int(splt[1])
      
      segments.append({"type":"AREA_INFLOW","prev":last_fwd + idx_offset,"xk":PLATFORM_LEN, "platform_id":platform_id})
      last_fwd = len(segments) - 1
      end_idx[0] = last_fwd + idx_offset

      
      if self.reversible:
        if last_rev != -1: segments[last_rev]["prev"] = len(segments) + idx_offset
        
        adj_i = len(segments)-1
        segments.append({"type":"AREA_OUTFLOW","next":last_rev + idx_offset,"xk":PLATFORM_LEN,"adjacent":adj_i + idx_offset, "platform_id":platform_id})
        segments[adj_i]["adjacent"] = len(segments)-1 + idx_offset
        last_rev = len(segments) - 1
        end_idx[1] = last_rev + idx_offset

    return segments, start_idx, end_idx

class SegmentJunction:
  def __init__(self, outflow_edges, outflow_idxs, dir):
    self.dir = dir
    self.outflow_idxs = outflow_idxs.copy()
    self.outflow_edges = outflow_edges.copy()
    pass

class EdgeManager:
  def __init__(self):
    self.edges : list[EdgeData] = []
    self.edge_starts = []
    self.edge_ends = []

    self.station_structure = []
    self.split_nodes : dict[tuple[tuple[int],tuple[int]], SegmentJunction] = {}

  def register_edge(self, edge : EdgeData):
    for e in self.edges:
      # Edge already exists
      if e.matches(edge.start_node.name, edge.end_node.name):
        e.forward_flows += edge.forward_flows
        e.reverse_flows += edge.reverse_flows
        return
      # Reversed edge exists, and one or the other can be reversed
      if (e.reversible or edge.reversible) and (e.matches(edge.end_node.name, edge.start_node.name)):
        e.forward_flows += edge.reverse_flows
        e.reverse_flows += edge.forward_flows
        return
    
    start = None; end = None
    for i,e in enumerate(self.edges):
      if e.start_node.name == edge.end_node.name:
        end = i
      if e.end_node.name == edge.start_node.name:
        start = i
      
    self.edges.append(edge)
    if start is None and end is None: # No connections needed
      l = len(self.station_structure)
      struc, start_idx, end_idx = edge.to_dict(l)

      self.station_structure.extend(struc)
      self.edge_starts.append(start_idx)
      self.edge_ends.append(end_idx)

      print(f"  Added edge from {self.edge_starts[-1]} to {self.edge_ends[-1]} in structure")
    elif start is not None and end is None:
      # Update the node that this wants to connect to
      existing_start_idx = self.edge_ends[start]
      
      # Fwd only
      if len(existing_start_idx) == 1:
        next = self.station_structure[existing_start_idx[0]]["next"]
        if next == -1: # No connection yet
          l = len(self.station_structure)
          self.station_structure[existing_start_idx[0]]["next"] = l
          struc, start_idx, end_idx = edge.to_dict(l)
          
          if len(start_idx) != len(existing_start_idx):
            raise Exception("Cannot join bi-directional edge to unidirectional edge")
          
          struc[start_idx[0]-l]["prev"] = existing_start_idx[0] # Change the prev field so it links

          self.station_structure.extend(struc)
          self.edge_starts.append(start_idx)
          self.edge_ends.append(end_idx)

          print(f"  Linked unidirectional edge from {self.edge_starts[-1]} to {self.edge_ends[-1]} in structure")
        else: # Already has a connection, reroute
          raise NotImplementedError()
      else:
        next = self.station_structure[existing_start_idx[0]]["next"]
        prev = self.station_structure[existing_start_idx[1]]["prev"]
        if next == -1 and prev == -1: # No connection yet
          l = len(self.station_structure)
          self.station_structure[existing_start_idx[0]]["next"] = l
          self.station_structure[existing_start_idx[1]]["prev"] = l+1
          struc, start_idx, end_idx = edge.to_dict(l)
          
          if len(start_idx) != len(existing_start_idx):
            raise Exception("Cannot join bi-directional edge to unidirectional edge")
          struc[start_idx[0]-l]["prev"] = existing_start_idx[0] # Change the fields so they link
          struc[start_idx[1]-l]["next"] = existing_start_idx[1]

          self.station_structure.extend(struc)
          self.edge_starts.append(start_idx)
          self.edge_ends.append(end_idx)
          
          print(f"  Linked bidirectional edge from {self.edge_starts[-1]} to {self.edge_ends[-1]} in structure")

        elif (next != -1) ^ (next != -1):
          raise Exception("Partial link error")
        else:
          l = len(self.station_structure)
          if "AREA" in self.station_structure[existing_start_idx[0]]["type"]:
            # Move back one level if the end is an area
            existing_start_idx[0] = self.station_structure[existing_start_idx[0]]["prev"]
            
          if "AREA" in self.station_structure[existing_start_idx[1]]["type"]:
            # Move back one level if the end is an area
            existing_start_idx[1] = self.station_structure[existing_start_idx[1]]["next"]

          self.station_structure.extend([
            {"type":"SPLIT_OUTPUT","id":l,"prev":existing_start_idx[0],"next":l+2,"adjacent":l+1, "secondary":l+4,"split_ratio":-1, "xk":SLICE_LEN},
            {"type":"SPLIT_INPUT","id":l+1,"prev":l+3,"next":existing_start_idx[1],"adjacent":l, "secondary":l+5, "xk":SLICE_LEN},
            {"type":"SPLIT_INPUT","id":l+2,"prev":l,"next":self.station_structure[existing_start_idx[0]]["next"],"adjacent":l+3, "secondary":l+5, "xk":SLICE_LEN},
            {"type":"SPLIT_OUTPUT","id":l+3,"prev":self.station_structure[existing_start_idx[1]]["prev"],"next":l+1,"adjacent":l+2, "secondary":l+4,"split_ratio":-1, "xk":SLICE_LEN},
            {"type":"SPLIT_INPUT","id":l+4,"prev":l,"next":-1,"adjacent":l+5, "secondary":l+3, "xk":SLICE_LEN},
            {"type":"SPLIT_OUTPUT","id":l+5,"prev":-1,"next":l+1,"adjacent":l+4, "secondary":l+2,"split_ratio":-1, "xk":SLICE_LEN},
          ])

          # ---------------------------------
          # Add to junction list

          start_edge_id_graph = self.__edges_id(start)
          next_edge_idx = None
          for i,e in enumerate(self.edges[:-1]):
            if e.start_node.name == edge.start_node.name:
              next_edge_idx = i
              break
          assert next_edge_idx is not None
          next_edge_id_graph = self.__edges_id(next_edge_idx)
          self_edge_id_graph = self.__edges_id(-1)

          if start_edge_id_graph in self.split_nodes:
            self.split_nodes[start_edge_id_graph].outflow_edges.append(self_edge_id_graph)
            self.split_nodes[start_edge_id_graph].outflow_idxs.append(l)
          else:
            self.split_nodes[start_edge_id_graph] = SegmentJunction([next_edge_id_graph, self_edge_id_graph],[l],"forward_flows")

          if next_edge_id_graph in self.split_nodes:
            self.split_nodes[next_edge_id_graph].outflow_edges.append(self_edge_id_graph)
            self.split_nodes[next_edge_id_graph].outflow_idxs.append(l+3)
          else:
            self.split_nodes[next_edge_id_graph] = SegmentJunction([start_edge_id_graph, self_edge_id_graph],[l+3],"reverse_flows")

          ## Below should never occur?
          #
          # if self_edge_id_graph in self.split_nodes:
          #   self.split_nodes[self_edge_id_graph].outflow_edges.append(next_edge_id_graph)
          #   self.split_nodes[self_edge_id_graph].outflow_idxs.append(l+5)
          # else:
          self.split_nodes[self_edge_id_graph] = SegmentJunction([start_edge_id_graph, next_edge_id_graph],[l+5],"reverse_flows")
          
          # ---------------------------------

          # Fill in linking parameters in structure
          self.station_structure[self.station_structure[existing_start_idx[0]]["next"]]["prev"] = l+2
          self.station_structure[self.station_structure[existing_start_idx[1]]["prev"]]["next"] = l+3
          self.station_structure[existing_start_idx[0]]["next"] = l
          self.station_structure[existing_start_idx[1]]["prev"] = l+1

          self.edge_ends[start] = existing_start_idx = [l+2,l+3] ##l+2 l+3 # Update to the new node for future joints

          struc, start_idx, end_idx = edge.to_dict(l+6)
          
          if len(start_idx) != len(existing_start_idx):
            raise Exception("Cannot join bi-directional edge to unidirectional edge")
          struc[start_idx[0]-l-6]["prev"] = l+4 # Change the fields so they link
          struc[start_idx[1]-l-6]["next"] = l+5
          
          self.station_structure[l+4]["next"] = start_idx[0]
          self.station_structure[l+5]["prev"] = start_idx[1]

          self.station_structure.extend(struc)
          self.edge_starts.append(start_idx)
          self.edge_ends.append(end_idx)
          print(f"  Linked bidirectional edge as junction from {self.edge_starts[-1]} to {self.edge_ends[-1]} in structure")
          
    elif end is not None and start is None:
      # Update the node that this wants to connect to
      existing_end_idx = self.edge_ends[end]
      
      # Fwd only
      if len(existing_end_idx) == 1:
        raise NotImplementedError()
      
      else:
        if "AREA" in self.station_structure[existing_end_idx[0]]["type"]:
          # Move back one level if the end is an area
          existing_end_idx[0] = self.station_structure[existing_end_idx[0]]["prev"]
          
        if "AREA" in self.station_structure[existing_end_idx[1]]["type"]:
          # Move back one level if the end is an area
          existing_end_idx[1] = self.station_structure[existing_end_idx[1]]["next"]

        next = self.station_structure[existing_end_idx[0]]["next"]
        prev = self.station_structure[existing_end_idx[1]]["prev"]
        if next == -1 and prev == -1: # No connection yet
          raise NotImplementedError("No clue how you got here, you probably have corrupted data or I missed something huge")
        elif (next != -1) ^ (next != -1):
          raise Exception("Partial link error")
        else:
          l = len(self.station_structure)

          self.station_structure.extend([
            {"type":"SPLIT_OUTPUT","id":l,"prev":existing_end_idx[0],"next":l+2,"adjacent":l+1, "secondary":l+4,"split_ratio":-1, "xk":SLICE_LEN},
            {"type":"SPLIT_INPUT","id":l+1,"prev":l+3,"next":existing_end_idx[1],"adjacent":l, "secondary":l+5, "xk":SLICE_LEN},
            {"type":"SPLIT_INPUT","id":l+2,"prev":l,"next":self.station_structure[existing_end_idx[0]]["next"],"adjacent":l+3, "secondary":l+5, "xk":SLICE_LEN},
            {"type":"SPLIT_OUTPUT","id":l+3,"prev":self.station_structure[existing_end_idx[1]]["prev"],"next":l+1,"adjacent":l+2, "secondary":l+4,"split_ratio":-1, "xk":SLICE_LEN},
            {"type":"SPLIT_INPUT","id":l+4,"prev":l,"next":-1,"adjacent":l+5, "secondary":l+3, "xk":SLICE_LEN},
            {"type":"SPLIT_OUTPUT","id":l+5,"prev":-1,"next":l+1,"adjacent":l+4, "secondary":l+2,"split_ratio":-1, "xk":SLICE_LEN},
          ])
          
          # ---------------------------------
          # Add to junction list

          start_edge_id_graph = self.__edges_id(end)
          next_edge_idx = None
          for i,e in enumerate(self.edges[:-1]):
            if e.end_node.name == edge.end_node.name:
              next_edge_idx = i
              break
          assert next_edge_idx is not None
          next_edge_id_graph = self.__edges_id(next_edge_idx)
          self_edge_id_graph = self.__edges_id(-1)

          if start_edge_id_graph in self.split_nodes:
            self.split_nodes[start_edge_id_graph].outflow_edges.append(self_edge_id_graph)
            self.split_nodes[start_edge_id_graph].outflow_idxs.append(l)
          else:
            self.split_nodes[start_edge_id_graph] = SegmentJunction([next_edge_id_graph, self_edge_id_graph],[l],"forward_flows")

          if next_edge_id_graph in self.split_nodes:
            self.split_nodes[next_edge_id_graph].outflow_edges.append(self_edge_id_graph)
            self.split_nodes[next_edge_id_graph].outflow_idxs.append(l+3)
          else:
            self.split_nodes[next_edge_id_graph] = SegmentJunction([start_edge_id_graph, self_edge_id_graph],[l+3],"reverse_flows")

          ## Below should never occur?
          #
          # if self_edge_id_graph in self.split_nodes:
          #   self.split_nodes[self_edge_id_graph].outflow_edges.append(next_edge_id_graph)
          #   self.split_nodes[self_edge_id_graph].outflow_idxs.append(l+5)
          # else:
          self.split_nodes[self_edge_id_graph] = SegmentJunction([start_edge_id_graph, next_edge_id_graph],[l+5],"reverse_flows")
          
          # ---------------------------------

          
          # Fill in linking parameters in structure
          self.station_structure[self.station_structure[existing_end_idx[0]]["next"]]["prev"] = l+2
          self.station_structure[self.station_structure[existing_end_idx[1]]["prev"]]["next"] = l+3
          self.station_structure[existing_end_idx[0]]["next"] = l
          self.station_structure[existing_end_idx[1]]["prev"] = l+1

          self.edge_ends[end] = existing_start_idx = [l+4,l+5] # Update to the new node for future joints

          struc, start_idx, end_idx = edge.to_dict(l+6)
          
          if len(start_idx) != len(existing_start_idx):
            raise Exception("Cannot join bi-directional edge to unidirectional edge")
          struc[end_idx[0]-l-6]["next"] = l+5 # Change the fields so they link
          struc[end_idx[1]-l-6]["prev"] = l+4
          
          self.station_structure[l+4]["next"] = end_idx[1]
          self.station_structure[l+5]["prev"] = end_idx[0]

          self.station_structure.extend(struc)
          self.edge_starts.append(start_idx)
          self.edge_ends.append(end_idx)
          print(f"  Linked bidirectional edge as junction from {self.edge_starts[-1]} to {self.edge_ends[-1]} in structure")
    else:
      raise Exception(f"{start} {end}")

  def calculate_splits(self, split_rates : list, flow_rates : pd.DataFrame):
    edge_flow_graph = nx.Graph()
    for edge in self.edges:
      edge_flow_graph.add_edge(*self.__edge_id(edge),forward_flows=edge.forward_flows,reverse_flows=edge.reverse_flows)

    # print(*edge_flow_graph.edges.data(),sep="\n")
    for id, junc in self.split_nodes.items():
      # print("Updating split node "+str(id))
      inflow = edge_flow_graph.get_edge_data(*id)[junc.dir]

      for i in range(len(junc.outflow_idxs)):
        primary_outflow = edge_flow_graph.get_edge_data(*junc.outflow_edges[i])[junc.dir]
        primary_outflow = [*set(i for i in primary_outflow if i in inflow)]
        # primary_outflow = [i for i in primary_outflow if i in inflow]
        split = [inflow, primary_outflow]
        print("  Split ("+str(junc.outflow_idxs[i])+"): "+str(split))
        
        if len(primary_outflow) == 0:
          self.station_structure[junc.outflow_idxs[i]]["split_ratio"] = 0
        elif primary_outflow == inflow:
          self.station_structure[junc.outflow_idxs[i]]["split_ratio"] = 1
        else:
          # Offset indexes by -2 for direct translation from excel rows (1 for 0 indexing, 1 for column headers)
          inflow_rate = np.sum(np.array([flow_rates.iloc[row - 2,19:] if isinstance(row, int) 
            else row[1]*flow_rates.iloc[row[0] - 2,19:] 
            for row in inflow]).astype(np.float32),axis=0)
          outflow_rate = np.sum(np.array([flow_rates.iloc[row - 2,19:] if isinstance(row, int) 
            else row[1]*flow_rates.iloc[row[0] - 2,19:] 
            for row in primary_outflow]).astype(np.float32),axis=0)
          flow_split = np.divide(outflow_rate, inflow_rate,out=np.zeros_like(outflow_rate), where=inflow_rate != 0)
          if np.all(flow_split == flow_split[0]):
            self.station_structure[junc.outflow_idxs[i]]["split_ratio"] = float(flow_split[0])
            continue
          # Log to file
          # print(*flow_split,sep=", ",file=split_rate_f)
          split_rates.append(flow_split)
          self.station_structure[junc.outflow_idxs[i]]["split_ratio"] = {"index":len(split_rates)-1}
          


        for x in edge_flow_graph.get_edge_data(*junc.outflow_edges[i])[junc.dir]:
          if x in inflow:
            inflow.remove(x)

  def __edges_id(self, i : int):
    return self.__edge_id(self.edges[i])
  @staticmethod
  def __edge_id(edge : EdgeData):
    return (edge.start_node.name, edge.end_node.name)


class JunctionInformation:
  def __init__(self):
    self.links = [0]*6

  def update_link(self, incoming, outgoing):
    self.links[incoming]
    

if len(sys.argv) == 1:
  path = "data-collection/station_structure/outputs/stations_mod.pkl"
else:
  path = sys.argv[1]

with open(path, "rb") as f:
  stations = pickle.load(f)
assert stations is not None

station_flows = pd.read_csv("data-collection/csv_sheet/Station_Flows/NBT23FRI_filtered.csv")
split_ratio_list = []

station_structures = {}

for station_id in paths.paths.keys():
  print(f"Station ID: {station_id}")

  station_paths = paths.paths[station_id]
  try:
    station = stations[station_id]
  except KeyError as e:
    print("  Key error on station data")
    continue

  station_segment_structure = EdgeManager()
  for path in station_paths:
    for i in range(len(path.sequence) - 1):
      # Find the type
      # print((path.sequence[i], path.sequence[i+1]))
      types = station.edges[(station.edges["start"] == path.sequence[i]) & (station.edges["end"] == path.sequence[i+1])]["type"].to_list()
      if len(types) == 0:
        print("Fatal - could not find type")
        continue
      elif len(types) == 1:
        path_type = types[0]
      else:
        # Precedence for modelling
        if "LEVEL" in types:
          path_type = "LEVEL"
        elif "ESCALATOR" in types:
          path_type = "ESCALATOR"
        elif "RAMP" in types:
          path_type = "RAMP"
        elif "STAIRS" in types:
          path_type = "RAMP"
        else:
          print("Fatal - could not find type")
          continue

      # Construct the edge
      e = EdgeData(station.nodes.loc(axis=0)[path.sequence[i]],station.nodes.loc(axis=0)[path.sequence[i+1]],
                   path.reverse_flows is not None,path_type, path.flow_rows,path.reverse_flows)
      station_segment_structure.register_edge(e)


  # Store JSON 
  station_segment_structure.calculate_splits(split_ratio_list,station_flows)
  station_structures[station_id] = {"initial_state":0}
  station_structures[station_id]["structure"] = copy.deepcopy(station_segment_structure.station_structure)
  print(f"  {len(station_structures[station_id]["structure"])} segments")

with open("split_ratios.csv","w+") as f:
  np.savetxt(f, np.array(split_ratio_list), delimiter=',',fmt="%.6f")
with open("station_output.json","w+") as f:
  print(json.dumps(station_structures),file=f)