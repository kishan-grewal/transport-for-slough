import json
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

with open("config/station_config_compressed.json","r") as f:
  station_config = json.load(f)
assert (station_config is not None)

i = 1
for station in station_config.keys():
  station_trunc = ' '.join(station.split(' ')[:-2])
  
  legend = []
  key_indexes = []; secondary_indexes = []
  for key in station_config[station]:
    if "Entrance" in key or "Platform" in key:
      legend.extend([key + " (1)", key + " (2)"])
      key_indexes.extend(station_config[station][key])
    else:
      secondary_indexes.extend(station_config[station][key])
      
  data = pd.read_csv(f"out/stations/{station_trunc}.csv").dropna(axis=1).to_numpy()
  
  fig = plt.figure(figsize=(12,9))
  plt.plot(data[:2000,key_indexes])
  plt.plot(data[:2000,secondary_indexes],'k--')
  plt.title(station_trunc)
  plt.legend(legend)
  plt.savefig(f"out/plots/{station_trunc}.jpg")
  plt.close()
  
  data = pd.read_csv(f"out/stations/{station_trunc} flows.csv").dropna(axis=1).to_numpy()
  offst = list(data[0,:]).index(-1)
  inflows = np.sum(data[:,:offst],axis=1)
  outflows = np.sum(data[:,offst+1:],axis=1)
  
  fig = plt.figure(figsize=(12,9))
  ax = plt.subplot(1,2,1)
  ax.set_title("Inflow rates")
  ax.bar(np.arange(len(inflows)),inflows)
  ax = plt.subplot(1,2,2)
  ax.set_title("Outflow rates")
  ax.bar(np.arange(len(outflows)),outflows)
  
  plt.title(station_trunc)
  plt.savefig(f"out/plots/{station_trunc} flows.jpg")
  plt.close()
  i += 1
  
plt.show()