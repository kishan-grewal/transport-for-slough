import paths
import sys
import pickle
import math
import copy
import itertools

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
SPLIT_R = -2

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

  def to_dict(self, idx_offset = 0, ignore_entrance : bool = False, ignore_platform : bool = False):
    segments = []
    start_idx = []
    end_idx = []

    platforms = {}
    entrances = {}

    last_fwd = -1
    last_rev = -1

    if not ignore_entrance and ("entrance" in self.start_node["nodeType"] or "exit" in self.start_node["nodeType"]):
      segments.append({"id":len(segments)+idx_offset,"type":"AREA_OUTFLOW","next":-1,"xk":1,"is_entrance":True})
      last_fwd = len(segments)-1
      start_idx.append(last_fwd + idx_offset)
      end_idx = [last_fwd + idx_offset]
      
      
      if self.reversible:
        segments.append({"id":len(segments)+idx_offset,"type":"AREA_INFLOW","prev":-1,"xk":2147483647})
        last_rev = len(segments)-1
        start_idx.append(last_rev + idx_offset)
        end_idx = [*end_idx, last_rev + idx_offset]
    else:
      start_idx = [idx_offset]
      end_idx = [-1]
      if self.reversible:
        start_idx = [idx_offset, 1 + idx_offset]
        end_idx = [-1,-1]
    
    distance = vincenty_sphere_distance(float(self.start_node["x"]),float(self.start_node["y"]),float(self.end_node["x"]),float(self.end_node["y"]))
    n_slices = max(1,math.ceil(distance / SLICE_LEN)) # Ensure there is always one segment, even for negligible length regions

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

    if "platform" in self.end_node["nodeType"] and not ignore_platform:
      if last_fwd != -1: segments[last_fwd]["next"] = len(segments) + idx_offset

      platform_id = "NONE"
      splt = self.end_node["nodeType"].split(' ')
      if len(splt) > 1:
        platform_id = int(splt[1])
      
      segments.append({"id":len(segments)+idx_offset,"type":"AREA_INFLOW","prev":last_fwd + idx_offset,"xk":PLATFORM_LEN, "platform_id":platform_id})
      last_fwd = len(segments) - 1
      end_idx[0] = last_fwd + idx_offset

      if platform_id in platforms:
        platforms[platform_id].append(len(segments)-1 + idx_offset)
      else:
        platforms[platform_id] = [len(segments)-1 + idx_offset]

      
      if self.reversible:
        if last_rev != -1: segments[last_rev]["prev"] = len(segments) + idx_offset
        
        adj_i = len(segments)-1
        segments.append({"id":len(segments)+idx_offset,"type":"AREA_OUTFLOW","next":last_rev + idx_offset,"xk":PLATFORM_LEN,"adjacent":adj_i + idx_offset, "platform_id":platform_id})
        segments[adj_i]["adjacent"] = len(segments)-1 + idx_offset
        platforms[platform_id].append(len(segments)-1 + idx_offset)
        last_rev = len(segments) - 1
        end_idx[1] = last_rev + idx_offset

    return segments, start_idx, end_idx, platforms

class SegmentJunction:
  def __init__(self, outflow_edges, outflow_idxs, dir, outflow_dirs):
    self.dir = dir
    self.outflow_idxs = outflow_idxs.copy()
    self.outflow_edges = outflow_edges.copy()
    self.outflow_dirs = outflow_dirs.copy()
    pass

class EdgeManager:
  def __init__(self):
    self.edges : list[EdgeData] = []
    self.edge_starts = []
    self.edge_ends = []

    self.station_structure = []
    self.split_nodes : dict[tuple[tuple[int],tuple[int]], SegmentJunction | list[SegmentJunction]] = {}

  def __start_node_juncs(self,l:int,start_edge_id_graph,next_edge_id_graph,self_edge_id_graph):
    if start_edge_id_graph in self.split_nodes:
      assert isinstance(self.split_nodes[start_edge_id_graph],SegmentJunction)
      self.split_nodes[start_edge_id_graph].outflow_edges.append(self_edge_id_graph) # type: ignore
      self.split_nodes[start_edge_id_graph].outflow_idxs.append(l) # type: ignore
      self.split_nodes[start_edge_id_graph].outflow_dirs.append("forward_flows") # type: ignore
    else:
      self.split_nodes[start_edge_id_graph] = SegmentJunction([next_edge_id_graph, self_edge_id_graph],[l],"forward_flows",["forward_flows","forward_flows"])

    if next_edge_id_graph in self.split_nodes:
      assert isinstance(self.split_nodes[next_edge_id_graph],SegmentJunction)
      self.split_nodes[next_edge_id_graph].outflow_edges.append(self_edge_id_graph) # type: ignore
      self.split_nodes[next_edge_id_graph].outflow_idxs.append(l+3) # type: ignore
      self.split_nodes[next_edge_id_graph].outflow_dirs.append("forward_flows" if self.split_nodes[next_edge_id_graph].dir == "reverse_flows" else "reverse_flows") # type: ignore
    else:
      self.split_nodes[next_edge_id_graph] = SegmentJunction([start_edge_id_graph, self_edge_id_graph],[l+3],"reverse_flows",["reverse_flows","forward_flows"])

  def __end_node_juncs(self,l:int,start_edge_id_graph,next_edge_id_graph,self_edge_id_graph):
    if start_edge_id_graph in self.split_nodes:
      assert isinstance(self.split_nodes[start_edge_id_graph],SegmentJunction)
      self.split_nodes[start_edge_id_graph].outflow_edges.insert(0,self_edge_id_graph) # type: ignore
      self.split_nodes[start_edge_id_graph].outflow_idxs.insert(0,l+3) # type: ignore
      self.split_nodes[start_edge_id_graph].outflow_dirs.insert(0,self.split_nodes[start_edge_id_graph].dir) # type: ignore
      # print(f"1. Added {l+3} to {start_edge_id_graph}")
    else:
      self.split_nodes[start_edge_id_graph] = SegmentJunction([next_edge_id_graph, self_edge_id_graph],[l+3],"forward_flows",["forward_flows","forward_flows"])
      # print(f"2. Created {start_edge_id_graph} with {l}")

    if next_edge_id_graph in self.split_nodes:
      assert isinstance(self.split_nodes[next_edge_id_graph],SegmentJunction)
      self.split_nodes[next_edge_id_graph].outflow_edges.append(self_edge_id_graph) # type: ignore
      self.split_nodes[next_edge_id_graph].outflow_idxs.append(l) # type: ignore
      self.split_nodes[next_edge_id_graph].outflow_dirs.append("reverse_flows" if self.split_nodes[next_edge_id_graph].dir == "reverse_flows" else "reverse_flows") # type: ignore
      # print(f"3. Added {l} to {next_edge_id_graph}")
    else:
      self.split_nodes[next_edge_id_graph] = SegmentJunction([start_edge_id_graph, self_edge_id_graph],[l],"reverse_flows",["reverse_flows","reverse_flows"])
      # print(f"4. Created {next_edge_id_graph} with {l}")

  def register_edge(self, edge : EdgeData):
    edge_platforms = {}
    for e in self.edges:
      # Edge already exists
      if e.matches(edge.start_node.name, edge.end_node.name):
        e.forward_flows += edge.forward_flows
        e.reverse_flows += edge.reverse_flows
        return {}
      # Reversed edge exists, and one or the other can be reversed
      if (e.reversible or edge.reversible) and (e.matches(edge.end_node.name, edge.start_node.name)):
        e.forward_flows += edge.reverse_flows
        e.reverse_flows += edge.forward_flows
        return {}
    
    start = None; end = None; prev = None; next = None
    for i,e in enumerate(self.edges):
      if e.start_node.name == edge.start_node.name:
        start = i
      if e.end_node.name == edge.end_node.name:
        end = i
      if e.end_node.name == edge.start_node.name:
        prev = i
      if e.start_node.name == edge.end_node.name:
        next = i
      
    self.edges.append(edge)
    print(f"\tS: {start} | E: {end} | P: {prev} | N: {next} | ({edge.start_node.name} - {edge.end_node.name})")
    if prev is None and start is None and end is None: # No connections needed
      l = len(self.station_structure)
      struc, start_idx, end_idx, platforms = edge.to_dict(l)
      edge_platforms.update(platforms)

      self.station_structure.extend(struc)
      self.edge_starts.append(start_idx)
      self.edge_ends.append(end_idx)

      print(f"  Added edge from {self.edge_starts[-1]} to {self.edge_ends[-1]} in structure")
    
    # Direct chain
    elif prev is not None and start is None and end is None:
      # Update the node that this wants to connect to
      existing_start_idx = self.edge_ends[prev]
      
      # Fwd only
      if len(existing_start_idx) == 1:
        l = len(self.station_structure)
        self.station_structure[existing_start_idx[0]]["next"] = l
        struc, start_idx, end_idx,platforms = edge.to_dict(l)
        edge_platforms.update(platforms)
        
        if len(start_idx) != len(existing_start_idx):
          raise Exception("Cannot join bi-directional edge to unidirectional edge")
        
        struc[start_idx[0]-l]["prev"] = existing_start_idx[0] # Change the prev field so it links

        self.station_structure.extend(struc)
        self.edge_starts.append(start_idx)
        self.edge_ends.append(end_idx)

        print(f"  Linked unidirectional edge from {self.edge_starts[-1]} to {self.edge_ends[-1]} in structure")
      # Bi-directional
      else:
        l = len(self.station_structure)
        
        if "AREA" in self.station_structure[existing_start_idx[0]]["type"]:
          raise Exception()

        else:
          self.station_structure[existing_start_idx[0]]["next"] = l
          self.station_structure[existing_start_idx[1]]["prev"] = l+1
          struc, start_idx, end_idx,platforms = edge.to_dict(l)
          edge_platforms.update(platforms)
          
          if len(start_idx) != len(existing_start_idx):
            raise Exception("Cannot join bi-directional edge to unidirectional edge")
          struc[start_idx[0]-l]["prev"] = existing_start_idx[0] # Change the fields so they link
          struc[start_idx[1]-l]["next"] = existing_start_idx[1]

          self.station_structure.extend(struc)
          self.edge_starts.append(start_idx)
          self.edge_ends.append(end_idx)
          
          print(f"  Linked bidirectional edge from {self.edge_starts[-1]} to {self.edge_ends[-1]} in structure")

    # Has a common starting node
    if start is not None and end is None:
      if prev is not None:
        existing_start_idx = self.edge_ends[prev]

        # Move forward one level if the start is an area
        if "AREA" in self.station_structure[existing_start_idx[0]]["type"]:
          existing_start_idx[0] = self.station_structure[existing_start_idx[0]]["prev"]
          existing_start_idx[1] = self.station_structure[existing_start_idx[1]]["next"]
        
        while "SPLIT" in self.station_structure[existing_start_idx[0]]["type"]:
          existing_start_idx[0] += 6
        while "SPLIT" in self.station_structure[existing_start_idx[1]]["type"]:
          existing_start_idx[1] += 6
      else:
        existing_start_idx = self.edge_starts[start]
        # Move forward one level if the start is an area
        if "AREA" in self.station_structure[existing_start_idx[0]]["type"]:
          existing_start_idx[0] = self.station_structure[existing_start_idx[0]]["next"]
          existing_start_idx[1] = self.station_structure[existing_start_idx[1]]["prev"]
        
        while "SPLIT" in self.station_structure[existing_start_idx[0]]["type"]:
          existing_start_idx[0] += 6
        while "SPLIT" in self.station_structure[existing_start_idx[1]]["type"]:
          existing_start_idx[1] += 6


      l = len(self.station_structure)

      self.station_structure.extend([
        {"type":"SPLIT_OUTPUT","id":l,"prev":existing_start_idx[0],"next":l+2,"adjacent":l+1, "secondary":l+4,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+1,"prev":l+3,"next":existing_start_idx[1],"adjacent":l, "secondary":l+5, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+2,"prev":l,"next":self.station_structure[existing_start_idx[0]]["next"],"adjacent":l+3, "secondary":l+5, "xk":SLICE_LEN},
        {"type":"SPLIT_OUTPUT","id":l+3,"prev":self.station_structure[existing_start_idx[1]]["prev"],"next":l+1,"adjacent":l+2, "secondary":l+4,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+4,"prev":l,"next":-1,"adjacent":l+5, "secondary":l+3, "xk":SLICE_LEN},
        {"type":"SPLIT_OUTPUT","id":l+5,"prev":-1,"next":l+1,"adjacent":l+4, "secondary":l+2,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
      ])

      # ---------------------------------
      # Add to junction list

      start_edge_id_graph = self.__edges_id(prev if prev is not None else start)
      next_edge_idx = None
      for i,e in enumerate(self.edges[:-1]):
        if e.start_node.name == edge.start_node.name:
          next_edge_idx = i
          break
      assert next_edge_idx is not None
      next_edge_id_graph = self.__edges_id(next_edge_idx)
      self_edge_id_graph = self.__edges_id(-1)

      self.__start_node_juncs(l,start_edge_id_graph,next_edge_id_graph,self_edge_id_graph)

      ## Below should never occur?
      #
      if self_edge_id_graph in self.split_nodes:
        raise Exception()
      else:
        self.split_nodes[self_edge_id_graph] = SegmentJunction([start_edge_id_graph, next_edge_id_graph],[l+5],"reverse_flows",["reverse_flows","forward_flows"])
      
      # ---------------------------------

      # Fill in linking parameters in structure
      print(self.station_structure[self.station_structure[existing_start_idx[0]]["next"]])
      print(self.station_structure[self.station_structure[existing_start_idx[1]]["prev"]])

      if self.station_structure[self.station_structure[existing_start_idx[0]]["next"]]["type"] == "SPLIT_INPUT": # Reversed order
        self.station_structure[self.station_structure[existing_start_idx[0]]["next"]]["next"] = l+2
        self.station_structure[self.station_structure[existing_start_idx[1]]["prev"]]["prev"] = l+3
      else:
        self.station_structure[self.station_structure[existing_start_idx[0]]["next"]]["prev"] = l+2
        self.station_structure[self.station_structure[existing_start_idx[1]]["prev"]]["next"] = l+3

      self.station_structure[existing_start_idx[0]]["next"] = l
      self.station_structure[existing_start_idx[1]]["prev"] = l+1

      struc, start_idx, end_idx,platforms = edge.to_dict(l+6, ignore_entrance=True, ignore_platform=next is not None)
      edge_platforms.update(platforms)
      
      if len(start_idx) != len(existing_start_idx):
        raise Exception("Cannot join bi-directional edge to unidirectional edge")
      struc[start_idx[0]-l-6]["prev"] = l+4
      struc[start_idx[1]-l-6]["next"] = l+5
      
      self.station_structure[l+4]["next"] = start_idx[0]
      self.station_structure[l+5]["prev"] = start_idx[1]

      self.station_structure.extend(struc)
      self.edge_starts.append(start_idx)
      self.edge_ends.append(end_idx)
      print(f"  Linked bidirectional edge as common starting junction from {self.edge_starts[-1]} to {self.edge_ends[-1]} in structure [{prev} {next}]")
          
    # Has a common ending node
    if start is None and end is not None:
      # Update the node that this wants to connect to
      if next is not None:
        existing_end_idx = self.edge_starts[next]
        
        while "SPLIT" in self.station_structure[existing_end_idx[0]]["type"]:
          existing_end_idx[0] += 6
        while "SPLIT" in self.station_structure[existing_end_idx[1]]["type"]:
          existing_end_idx[1] += 6
      else:
        existing_end_idx = self.edge_ends[end]
        if "AREA" in self.station_structure[existing_end_idx[0]]["type"]:
          existing_end_idx[0] = self.station_structure[existing_end_idx[0]]["prev"]
          existing_end_idx[1] = self.station_structure[existing_end_idx[1]]["next"]
          
        while "SPLIT" in self.station_structure[existing_end_idx[0]]["type"]:
          existing_end_idx[0] += 6
        while "SPLIT" in self.station_structure[existing_end_idx[1]]["type"]:
          existing_end_idx[1] += 6

      l = len(self.station_structure)

      if prev is not None:
        # Directly connect self to previous (no junction needed, or we would have a start value)
        # This will get overwritten by the following code, but is necesarry to correctly link the junction
        tmp_end_idx = self.edge_ends[prev]
        self.station_structure[tmp_end_idx[0]]["next"] = l
        self.station_structure[tmp_end_idx[1]]["prev"] = l+1

      # print(self.station_structure[existing_end_idx[0]]["prev"],self.station_structure[existing_end_idx[1]]["next"])
      self.station_structure.extend([
        {"type":"SPLIT_OUTPUT","id":l,"prev":existing_end_idx[0],"next":l+2,"adjacent":l+1, "secondary":l+4,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+1,"prev":l+3,"next":existing_end_idx[1],"adjacent":l, "secondary":l+5, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+2,"prev":l,"next":self.station_structure[existing_end_idx[0]]["next"],"adjacent":l+3, "secondary":l+5, "xk":SLICE_LEN},
        {"type":"SPLIT_OUTPUT","id":l+3,"prev":self.station_structure[existing_end_idx[1]]["prev"],"next":l+1,"adjacent":l+2, "secondary":l+4,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+4,"prev":l,"next":-1,"adjacent":l+5, "secondary":l+3, "xk":SLICE_LEN},
        {"type":"SPLIT_OUTPUT","id":l+5,"prev":-1,"next":l+1,"adjacent":l+4, "secondary":l+2,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
      ])
      
      # ---------------------------------
      # Add to junction list

      start_edge_id_graph = self.__edges_id(next if next is not None else end)
      next_edge_idx = None
      for i,e in enumerate(self.edges[:-1]):
        if e.end_node.name == edge.end_node.name:
          next_edge_idx = i
          break
      assert next_edge_idx is not None
      next_edge_id_graph = self.__edges_id(next_edge_idx)
      self_edge_id_graph = self.__edges_id(-1)

      self.__end_node_juncs(l,start_edge_id_graph,next_edge_id_graph,self_edge_id_graph)
      ## Below should never occur?
      #
      if self_edge_id_graph in self.split_nodes:
        raise Exception()
      else:
        self.split_nodes[self_edge_id_graph] = SegmentJunction([start_edge_id_graph, next_edge_id_graph],[l+5],"forward_flows",["reverse_flows","forward_flows"])

      # ---------------------------------

      # Fill in linking parameters in structure
      print(self.station_structure[self.station_structure[existing_end_idx[0]]["next"]])
      print(self.station_structure[self.station_structure[existing_end_idx[1]]["prev"]])

      self.station_structure[self.station_structure[existing_end_idx[0]]["next"]]["prev"] = l+2
      self.station_structure[self.station_structure[existing_end_idx[1]]["prev"]]["next"] = l+3
      self.station_structure[existing_end_idx[0]]["next"] = l
      self.station_structure[existing_end_idx[1]]["prev"] = l+1

      struc, start_idx, end_idx,platforms = edge.to_dict(l+6, ignore_entrance=prev is not None, ignore_platform=True)
      edge_platforms.update(platforms)
      
      if len(start_idx) != len(existing_end_idx):
        raise Exception("Cannot join bi-directional edge to unidirectional edge")
      
      struc[end_idx[0]-l-6]["next"] = l+5
      struc[end_idx[1]-l-6]["prev"] = l+4
      if prev is not None: # Connect to prev (i.e disconnect old connection)
        struc[end_idx[0]-l-6]["prev"] = self.edge_ends[prev][0]
        struc[end_idx[1]-l-6]["next"] = self.edge_ends[prev][1]
        
        self.station_structure[self.edge_ends[prev][0]]["next"] = l+6
        self.station_structure[self.edge_ends[prev][1]]["prev"] = l+7
      
      self.station_structure[l+4]["next"] = end_idx[1]
      self.station_structure[l+5]["prev"] = end_idx[0]

      self.station_structure.extend(struc)
      self.edge_starts.append(start_idx)
      self.edge_ends.append(end_idx)
      print(f"  Linked bidirectional edge as common ending junction from {self.edge_starts[-1]} to {self.edge_ends[-1]} in structure [{prev} {next}]")

    # Joins a pair of nodes
    if start is not None and end is not None:
      l = len(self.station_structure)

      if prev is not None:
        existing_start_idx = self.edge_ends[prev]
      else:
        existing_start_idx = self.edge_starts[start]
        
        # Move forward one level if the start is an area
        if "AREA" in self.station_structure[existing_start_idx[0]]["type"]:
          existing_start_idx[0] = self.station_structure[existing_start_idx[0]]["next"]
        if "AREA" in self.station_structure[existing_start_idx[1]]["type"]:
          existing_start_idx[1] = self.station_structure[existing_start_idx[1]]["prev"]
          
        while "SPLIT" in self.station_structure[existing_start_idx[0]]["type"]:
          existing_start_idx[0] += 6
        while "SPLIT" in self.station_structure[existing_start_idx[1]]["type"]:
          existing_start_idx[1] += 6
        
      if next is not None:
        existing_end_idx = self.edge_starts[next]
      else:
        existing_end_idx = self.edge_ends[end]
        if "AREA" in self.station_structure[existing_end_idx[0]]["type"]:
          existing_end_idx[0] = self.station_structure[existing_end_idx[0]]["prev"]
        if "AREA" in self.station_structure[existing_end_idx[1]]["type"]:
          existing_end_idx[1] = self.station_structure[existing_end_idx[1]]["next"]
             
        while "SPLIT" in self.station_structure[existing_end_idx[0]]["type"]:
          existing_end_idx[0] += 6
        while "SPLIT" in self.station_structure[existing_end_idx[1]]["type"]:
          existing_end_idx[1] += 6
      # print(self.station_structure[existing_start_idx[0]],self.station_structure[existing_start_idx[1]], prev)
      # print(self.station_structure[existing_end_idx[0]],self.station_structure[existing_end_idx[1]],next)
      self.station_structure.extend([
        {"type":"SPLIT_OUTPUT","id":l,"prev":existing_start_idx[0],"next":l+2,"adjacent":l+1, "secondary":l+4,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+1,"prev":l+3,"next":existing_start_idx[1],"adjacent":l, "secondary":l+5, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+2,"prev":l,"next":self.station_structure[existing_start_idx[0]]["prev"],"adjacent":l+3, "secondary":l+5, "xk":SLICE_LEN},
        {"type":"SPLIT_OUTPUT","id":l+3,"prev":self.station_structure[existing_start_idx[1]]["next"],"next":l+1,"adjacent":l+2, "secondary":l+4,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+4,"prev":l,"next":-1,"adjacent":l+5, "secondary":l+3, "xk":SLICE_LEN},
        {"type":"SPLIT_OUTPUT","id":l+5,"prev":-1,"next":l+1,"adjacent":l+4, "secondary":l+2,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
        
        {"type":"SPLIT_OUTPUT","id":l+6,"prev":existing_end_idx[0],"next":l+8,"adjacent":l+7, "secondary":l+10,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+7,"prev":l+9,"next":existing_end_idx[1],"adjacent":l+6, "secondary":l+11, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+8,"prev":l+6,"next":self.station_structure[existing_end_idx[0]]["next"],"adjacent":l+9, "secondary":l+11, "xk":SLICE_LEN},
        {"type":"SPLIT_OUTPUT","id":l+9,"prev":self.station_structure[existing_end_idx[1]]["prev"],"next":l+7,"adjacent":l+8, "secondary":l+10,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
        {"type":"SPLIT_INPUT","id":l+10,"prev":l+6,"next":-1,"adjacent":l+11, "secondary":l+9, "xk":SLICE_LEN},
        {"type":"SPLIT_OUTPUT","id":l+11,"prev":-1,"next":l+7,"adjacent":l+4, "secondary":l+8,"split_ratio":SPLIT_R, "xk":SLICE_LEN},
      ])

      
      # ---------------------------------
      # Add to junction list

      ## Start
      start_edge_id_graph = self.__edges_id(prev if prev is not None else start)
      next_edge_idx = None
      for i,e in enumerate(self.edges[:-1]):
        if e.start_node.name == edge.start_node.name:
          next_edge_idx = i
          break
      assert next_edge_idx is not None
      next_edge_id_graph = self.__edges_id(next_edge_idx)
      self_edge_id_graph = self.__edges_id(-1)
      self.__start_node_juncs(l,start_edge_id_graph,next_edge_id_graph,self_edge_id_graph)

      ## End
      start_edge_id_graph = self.__edges_id(next if next is not None else end)
      next_edge_idx = None
      for i,e in enumerate(self.edges[:-1]):
        if e.end_node.name == edge.end_node.name:
          next_edge_idx = i
          break
      assert next_edge_idx is not None
      next_edge_id_graph = self.__edges_id(next_edge_idx)
      self_edge_id_graph = self.__edges_id(-1)
      self.__end_node_juncs(l+6,start_edge_id_graph,next_edge_id_graph,self_edge_id_graph)

      ## Self (must be list of 2, bcs of having joints at both ends. This is a unique case, 
      # and things (should) never join to this edge) so this should be safe

      if self_edge_id_graph in self.split_nodes:
        raise Exception()
      else:
        self.split_nodes[self_edge_id_graph] = [SegmentJunction([start_edge_id_graph, next_edge_id_graph],[l+11],"reverse_flows",["reverse_flows","forward_flows"]) ,SegmentJunction([start_edge_id_graph, next_edge_id_graph],[l+5],"forward_flows",["reverse_flows","forward_flows"]), ]


      # Fill in linking parameters in structure
      self.station_structure[self.station_structure[existing_start_idx[0]]["prev"]]["next"] = l+2
      self.station_structure[self.station_structure[existing_start_idx[1]]["next"]]["prev"] = l+3
      self.station_structure[existing_start_idx[0]]["prev"] = l
      self.station_structure[existing_start_idx[1]]["next"] = l+1

      # Fill in linking parameters in structure
      self.station_structure[self.station_structure[existing_end_idx[0]]["next"]]["prev"] = l+8
      self.station_structure[self.station_structure[existing_end_idx[1]]["prev"]]["next"] = l+9
      self.station_structure[existing_end_idx[0]]["next"] = l+6
      self.station_structure[existing_end_idx[1]]["prev"] = l+7

      struc, start_idx, end_idx,platforms = edge.to_dict(l+12, ignore_entrance=prev is None, ignore_platform=next is None)
      edge_platforms.update(platforms)
      
      if len(start_idx) != len(existing_start_idx):
        raise Exception("Cannot join bi-directional edge to unidirectional edge")
      
      ## Start
      struc[start_idx[0]-l-12]["prev"] = l+4 # Change the fields so they link
      struc[start_idx[1]-l-12]["next"] = l+5
      
      self.station_structure[l+4]["next"] = start_idx[0]
      self.station_structure[l+5]["prev"] = start_idx[1]

      ## End
      struc[end_idx[0]-l-12]["next"] = l+11 # Change the fields so they link
      struc[end_idx[1]-l-12]["prev"] = l+10
      
      self.station_structure[l+10]["next"] = end_idx[1]
      self.station_structure[l+11]["prev"] = end_idx[0]

      self.station_structure.extend(struc)
      self.edge_starts.append(start_idx)
      self.edge_ends.append(end_idx)
      print(f"  Linked bidirectional edge between junctions from {self.edge_starts[-1]} to {self.edge_ends[-1]} in structure [{prev} {next}]")
    
    return edge_platforms
  
  def __update_junction_nodes(self, edge_flow_graph, id, junc : SegmentJunction, split_rates : list, flow_rates : pd.DataFrame):
    print("Updating split node "+str(id))
    inflow = edge_flow_graph.get_edge_data(*id)[junc.dir]#.copy()

    for i in range(len(junc.outflow_idxs)):
      primary_outflow = edge_flow_graph.get_edge_data(*junc.outflow_edges[i]).copy()
      # print(primary_outflow, junc.outflow_dirs[i])
      primary_outflow = [*set(n for n in primary_outflow[junc.outflow_dirs[i]] if n in inflow)]
      split = [inflow, primary_outflow]
      print("  Split ("+str(junc.outflow_idxs[i])+"): "+str(split))
      
      if len(primary_outflow) == 0:
        self.station_structure[junc.outflow_idxs[i]]["split_ratio"] = 0# if junc.dir != "reverse_flows" else 1
      elif primary_outflow == inflow:
        self.station_structure[junc.outflow_idxs[i]]["split_ratio"] = 1# if junc.dir != "reverse_flows" else 0
      else:
        # [print(f"    {flow_rates.iloc[row - 2,[4,5,8,9]]}") if isinstance(row, int) else print(f"    {flow_rates.iloc[row[0] - 2,[4,5,8,9]]}") for row in inflow]
        # Offset indexes by -2 for direct translation from excel rows (1 for 0 indexing, 1 for column headers)]
        inflow_rate = np.sum(np.array([flow_rates.iloc[row - 2,18:] if isinstance(row, int) 
          else row[1]*flow_rates.iloc[row[0] - 2,18:] 
          for row in inflow]).astype(np.float32),axis=0)
        outflow_rate = np.sum(np.array([flow_rates.iloc[row - 2,18:] if isinstance(row, int) 
          else row[1]*flow_rates.iloc[row[0] - 2,18:] 
          for row in primary_outflow]).astype(np.float32),axis=0)
        
        flow_split = np.divide(outflow_rate, inflow_rate,out=np.zeros_like(outflow_rate), where=inflow_rate != 0)
        # if junc.dir == "reverse_flows":
        #   flow_split = np.abs(1 - flow_split) # Clean up -0 values

        if np.all(np.round(flow_split,6) == flow_split[0]):
          self.station_structure[junc.outflow_idxs[i]]["split_ratio"] = float(flow_split[0])
        else:
          # Log to file
          # print(*flow_split,sep=", ",file=split_rate_f)
          split_rates.append(flow_split)
          self.station_structure[junc.outflow_idxs[i]]["split_ratio"] = {"index":len(split_rates)-1}
        


      # for x, rev_x in zip(edge_flow_graph.get_edge_data(*junc.outflow_edges[i])[junc.outflow_dirs[i]],
      #   edge_flow_graph.get_edge_data(*junc.outflow_edges[i])["reverse_flows" if junc.outflow_dirs[i] == "forward_flows" else "forward_flows"]):
      #   if x in inflow:
      #     inflow.append(rev_x)
      #     inflow.remove(x)
      for x,rev_x in zip(primary_outflow,edge_flow_graph.get_edge_data(*junc.outflow_edges[i])["reverse_flows" if junc.outflow_dirs[i] == "forward_flows" else "forward_flows"]):
        if x in inflow:
          inflow.remove(x)
          inflow.append(rev_x)

  def calculate_splits(self, split_rates : list, flow_rates : pd.DataFrame):
    edge_flow_graph = nx.DiGraph()
    for edge in self.edges:
      edge_flow_graph.add_edge(*self.__edge_id(edge),forward_flows=edge.forward_flows,reverse_flows=edge.reverse_flows)

    # print(*edge_flow_graph.edges.data(),sep="\n")
    for id, junc in self.split_nodes.items():
      if isinstance(junc,SegmentJunction):
        self.__update_junction_nodes(edge_flow_graph,id,junc,split_rates,flow_rates)
      else:
        for j in junc:
          self.__update_junction_nodes(edge_flow_graph,id,j,split_rates,flow_rates)

  def __edges_id(self, i : int):
    return self.__edge_id(self.edges[i])
  @staticmethod
  def __edge_id(edge : EdgeData):
    return (edge.start_node.name, edge.end_node.name)
    

if len(sys.argv) == 1:
  path = "data-collection/station_structure/outputs/stations_mod.pkl"
else:
  path = sys.argv[1]

with open(path, "rb") as f:
  stations = pickle.load(f)
assert stations is not None

station_flows = pd.read_csv("data/presentation-data/csv_sheet/Station_Flows/NBT24FRI_filtered.csv")
split_ratio_list = []

station_structures = {}
station_platforms = {}

for station_id in paths.paths.keys(): # 
  print(f"Station ID: {station_id}")

  station_paths = paths.paths[station_id]
  try:
    station = stations[station_id]
  except KeyError as e:
    print("  Key error on station data")
    continue

  station_segment_structure = EdgeManager()
  platforms = {}
  for path in station_paths:
    for i in range(len(path.sequence) - 1):
      # Find the type
      # print((path.sequence[i], path.sequence[i+1]))
      types = station.edges[(station.edges["start"] == path.sequence[i]) & (station.edges["end"] == path.sequence[i+1])]["type"].to_list()
      if len(types) == 0:
        print(f"Fatal - could not find type {path.sequence[i]} {path.sequence[i+1]}")
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
      
      new_platforms = station_segment_structure.register_edge(e)
      c = [platforms, new_platforms]
      platforms = {key: list(itertools.chain.from_iterable([d.get(key,[]) for d in c])) for key in set().union(*c)}


  # Store JSON 
  station_segment_structure.calculate_splits(split_ratio_list,station_flows)
  station_structures[station_id] = {"initial_state":0}
  station_structures[station_id]["structure"] = copy.deepcopy(station_segment_structure.station_structure)
  station_platforms[station_id] = platforms
  print(f"  {len(station_structures[station_id]["structure"])} segments")

with open("station_split_ratios.csv","w+") as f:
  np.savetxt(f, np.array(split_ratio_list), delimiter=',',fmt="%.6f")
with open("station_structures.json","w+") as f:
  print(json.dumps(station_structures),file=f)
with open("station_config.json","w+") as f:
  print(json.dumps(station_platforms),file=f)