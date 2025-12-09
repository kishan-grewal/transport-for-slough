# from station_structure import Station_Structure
# from retrieve_data import visualise_station
# import matplotlib.pyplot as plt
# import pickle
# import sys

# if len(sys.argv) == 1:
#   path = "data-collection/station_structure/outputs/stations.pkl"
# else:
#   path = sys.argv[1]

# with open(path, "rb") as f:
#   stations = pickle.load(f)
#   if not isinstance (stations,dict):
#     raise RuntimeError("Invalid load")

# cur_station = ""
# while(1):
#   cmd = input().split(' ',1)

#   try:
#     if cmd[0] == "x":
#       break
#     if cmd[0] == "s":
#       with open(f"data-collection/station_structure/outputs/{cmd[1]}.pkl", "rb") as f:
#         pickle.dump(stations, f)

#     if cmd[0] == "l":
#       if len(cmd) > 1:
#         if cmd[1] == "--help":
#           print(stations.keys())
#         else:
#           cur_station = cmd[1]
#       else:
#         cur_station = ""
#       print(f"Loaded station: [{cur_station}]")
#     if cmd[0] == "d":
#       key = cmd[1][1:-1].split(',')
#       key = (int(key[0]), int(key[1]))
#       print(key)
#       stations[cur_station].nodes.drop(key,axis=0)
#       stations[cur_station].edges.drop(stations[(stations[cur_station].edges["start"] == cmd[1]) | (stations[cur_station].edges["end"] == cmd[1])].index,axis=0)

#     if cmd[0] == "v":
#       visualise_station(stations[cur_station],"")
#       plt.show()
#     if cmd[0] == "p":
#       print(stations[cur_station].nodes)
#       print(stations[cur_station].edges)
#   except Exception as e:
#     print(f"Error {repr(e)}")

import sys
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QComboBox, QTableWidget, QTableWidgetItem, QPushButton, QHBoxLayout, QFileDialog, QHeaderView
from PyQt5.QtCore import Qt
import pandas as pd


from station_structure import Station_Structure
from retrieve_data import visualise_station
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import pickle
import sys

if len(sys.argv) == 1:
  path = "data-collection/station_structure/outputs/stations.pkl"
else:
  path = sys.argv[1]

with open(path, "rb") as f:
  stations = pickle.load(f)
  if not isinstance (stations,dict):
    raise RuntimeError("Invalid load")

class GraphEditor(QWidget):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Station Graph Editor")
        self.setGeometry(100, 100, 1100, 600)

        # ------------------------------------------------------
        # MAIN LAYOUT SPLIT: left side = tables, right = plot
        # ------------------------------------------------------
        self._main_layout = QHBoxLayout()
        self._left_layout = QVBoxLayout()
        self._main_layout.addLayout(self._left_layout, stretch=1)
        self.setLayout(self._main_layout)

        # Station selector
        self.graph_selector = QComboBox()
        self.graph_selector.addItems(stations.keys())
        self.graph_selector.currentIndexChanged.connect(self.load_graph_data)

        # Tables
        self.node_table = QTableWidget(self)
        self.edge_table = QTableWidget(self)

        # Buttons
        self.node_delete_button = QPushButton("Delete Node Row")

        self.node_delete_button.clicked.connect(self.delete_node_row)
        
        self.save_button = QPushButton("Save", self)
        self.save_button.clicked.connect(self.on_save)

        # Left side widgets
        self._left_layout.addWidget(self.graph_selector)
        self._left_layout.addWidget(self.node_table)
        self._left_layout.addWidget(self.node_delete_button)
        self._left_layout.addWidget(self.edge_table)
        self._left_layout.addWidget(self.save_button)

        # ------------------------------------------------------
        # Matplotlib Figure on the RIGHT
        # ------------------------------------------------------
        self.figure = Figure(figsize=(5, 5))
        self.canvas = FigureCanvas(self.figure)
        self._main_layout.addWidget(self.canvas, stretch=1)
        
        # Load the initial graph data (first item in dictionary)
        self.load_graph_data()

    def load_graph_data(self):
        # Get the selected graph object from dictionary
        selected_graph = stations[self.graph_selector.currentText()]

        # Load the nodes dataframe into the node table
        self.load_dataframe_to_table(selected_graph.nodes, self.node_table)

        # Load the edges dataframe into the edge table
        self.load_dataframe_to_table(selected_graph.edges, self.edge_table)
        self.update_plot()

    def update_plot(self):
        """Redraw the graph on the matplotlib canvas."""
        self.figure.clear()
        visualise_station(self.figure, stations[self.graph_selector.currentText()],self.graph_selector.currentText(), label_nodes=True)
        self.canvas.draw()

    def load_dataframe_to_table(self, dataframe, table_widget):
        # Set number of rows and columns
        table_widget.setRowCount(dataframe.shape[0])
        table_widget.setColumnCount(dataframe.shape[1])

        # Set headers
        table_widget.setHorizontalHeaderLabels(dataframe.columns)

        # Make the table editable
        for i in range(dataframe.shape[0]):
            for j in range(dataframe.shape[1]):
                item = QTableWidgetItem(str(dataframe.iloc[i, j]))
                item.setFlags(item.flags() | Qt.ItemIsEditable)  # Make cell editable
                table_widget.setItem(i, j, item)

        # Resize columns to fit content
        table_widget.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)

    def delete_node_row(self):
        row = self.node_table.currentRow()
        if row == -1:
            return

        s = stations[self.graph_selector.currentText()]

        # Get node ID from the dataframe
        node_id = s.nodes.iloc[row].name
        # Drop the node
        s.nodes = s.nodes.drop(s.nodes.index[row])

        # Drop edges where start or end == node_id
        s.edges = s.edges[
            (s.edges["start"] != node_id) &
            (s.edges["end"] != node_id)
        ]

        # Refresh tables
        self.load_graph_data()

    def on_save(self):
       with open(path[:-4] + "_mod.pkl", "wb") as f:
          pickle.dump(stations, f)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = GraphEditor()
    window.show()
    sys.exit(app.exec_())