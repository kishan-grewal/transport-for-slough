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
import copy

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

        self._main_layout = QHBoxLayout()
        self._left_layout = QVBoxLayout()
        self._main_layout.addLayout(self._left_layout, stretch=1)
        self.setLayout(self._main_layout)

        self.graph_selector = QComboBox()
        self.graph_selector.addItems(stations.keys())
        self.graph_selector.currentIndexChanged.connect(self.load_graph_data)

        self.node_table = QTableWidget(self)
        self.edge_table = QTableWidget(self)

        self.node_delete_button = QPushButton("Delete Node Row")
        self.node_delete_button.clicked.connect(self.delete_node_row)
        self.save_button = QPushButton("Save", self)
        self.save_button.clicked.connect(self.on_save)
        
        # Local copy data
        self.local_station = None

        self.confirm_button = QPushButton("Confirm Changes")
        self.confirm_button.clicked.connect(self.on_confirm)

        # Left side widgets
        self._left_layout.addWidget(self.graph_selector)
        self._left_layout.addWidget(self.node_table)
        self._left_layout.addWidget(self.node_delete_button)
        self._left_layout.addWidget(self.edge_table)
        self._left_layout.addWidget(self.confirm_button)
        self._left_layout.addWidget(self.save_button)


        self.figure = Figure(figsize=(5, 5))
        self.canvas = FigureCanvas(self.figure)
        self._main_layout.addWidget(self.canvas, stretch=1)
        
        # Load the initial graph data (first item in dictionary)
        self.load_graph_data()

    def load_graph_data(self):    
        self.local_station = copy.deepcopy(stations[self.graph_selector.currentText()])
        if self.local_station is None:
           return

        self.load_dataframe_to_table(self.local_station.nodes, self.node_table)
        self.load_dataframe_to_table(self.local_station.edges, self.edge_table)

        self.update_plot()


    def update_plot(self):
        self.figure.clear()
        
        if self.local_station is None:
           return
        visualise_station(self.figure, self.local_station ,self.graph_selector.currentText(), label_nodes=True)
        self.canvas.draw()
        
    def load_dataframe_to_table(self, dataframe, table_widget):
        index_vals = list(dataframe.index)

        # Column count = index column + dataframe columns
        table_widget.setRowCount(dataframe.shape[0])
        table_widget.setColumnCount(dataframe.shape[1] + 1)

        # Build headers: first column is "Index"
        headers = ["Index"] + list(dataframe.columns)
        table_widget.setHorizontalHeaderLabels(headers)

        for row in range(dataframe.shape[0]):

            # --- Fill index column ---
            idx_val = index_vals[row]
            # Convert MultiIndex tuples to a string
            if isinstance(idx_val, tuple):
                idx_str = str(idx_val)
            else:
                idx_str = str(idx_val)

            index_item = QTableWidgetItem(idx_str)
            index_item.setFlags(index_item.flags() & ~Qt.ItemIsEditable)  # read-only
            table_widget.setItem(row, 0, index_item)

            # --- Fill DataFrame values ---
            for col in range(dataframe.shape[1]):
                val = dataframe.iloc[row, col]
                item = QTableWidgetItem(str(val))
                item.setFlags(item.flags() | Qt.ItemIsEditable)
                table_widget.setItem(row, col + 1, item)

        table_widget.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)

    def delete_node_row(self):
        if self.local_station is None:
           return
        
        row = self.node_table.currentRow()
        if row == -1:
            return

        node_index = self.local_station.nodes.index[row]

        # Delete from local copies
        self.local_station.nodes = self.local_station.nodes.drop(node_index)

        self.local_station.edges = self.local_station.edges[
            (self.local_station.edges["start"] != node_index) &
            (self.local_station.edges["end"] != node_index)
        ]
        self.local_station.drop_disconnected()

        # Refresh UI from local copies
        self.load_dataframe_to_table(self.local_station.nodes, self.node_table)
        self.load_dataframe_to_table(self.local_station.edges, self.edge_table)
        self.update_plot()

    def on_confirm(self):
        """Write local edited DataFrames back into the stations dict."""
        stations[self.graph_selector.currentText()] = self.local_station

        print(f"Changes confirmed for {self.graph_selector.currentText()}.")

        self.update_plot()

    def on_save(self):
       with open(path[:-4] + "_mod.pkl", "wb") as f:
          pickle.dump(stations, f)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = GraphEditor()
    window.show()
    sys.exit(app.exec_())