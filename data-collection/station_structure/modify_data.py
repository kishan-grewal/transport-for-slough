import sys
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QComboBox, QTableView, QTableWidgetItem, QPushButton, QHBoxLayout, QFileDialog, QHeaderView
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
  
from PyQt5.QtCore import Qt, QAbstractTableModel, QModelIndex, QVariant
from PyQt5.QtGui import QColor

from PyQt5.QtWidgets import QStyledItemDelegate, QLineEdit

class DataFrameDelegate(QStyledItemDelegate):
    def setEditorData(self, editor, index):
        """Ensure the editor starts with the existing cell content."""
        value = index.model().data(index, Qt.ItemDataRole.EditRole) # type: ignore
        if isinstance(editor, QLineEdit):
            editor.setText(value)
        else:
            super().setEditorData(editor, index)

    def setModelData(self, editor, model, index):
        """Send edited value back to the model."""
        if isinstance(editor, QLineEdit):
            model.setData(index, editor.text(), Qt.ItemDataRole.EditRole) # type: ignore
        else:
            super().setModelData(editor, model, index)

class PandasModel(QAbstractTableModel):
    def __init__(self, df, editable=True):
        super().__init__()
        self._df = df
        self._editable = editable
        self._dirty_cells = set()  # (row, col)

    def rowCount(self, parent=None):
        return self._df.shape[0]

    def columnCount(self, parent=None):
        # +1 for index column
        return self._df.shape[1] + 1
    
    def data(self, index, role=Qt.ItemDataRole.DisplayRole):
        if not index.isValid():
            return QVariant()

        row = index.row()
        col = index.column()

        # --- Get value ---
        if col == 0:
            value = self._df.index[row]
            # Convert numpy ints in tuple or scalar to plain Python ints
            if isinstance(value, tuple):
                value = tuple(int(x) if hasattr(x, "item") else x for x in value)
            elif hasattr(value, "item"):  # single numpy scalar
                value = int(value)
        else:
            value = self._df.iat[row, col - 1]
            # Optional: convert numpy scalars to Python types
            if hasattr(value, "item"):
                value = value.item()

        # ---- Display text ----
        if role == Qt.ItemDataRole.DisplayRole or role == Qt.ItemDataRole.EditRole:
            return str(value)

        # ---- Highlight dirty cells ----
        if role == Qt.ItemDataRole.BackgroundRole:
            if (row, col) in self._dirty_cells:
                return QColor("yellow")

        return QVariant()



    # ----------- Header Labels -----------
    def headerData(self, section, orientation, role=Qt.ItemDataRole.DisplayRole):
        if role != Qt.ItemDataRole.DisplayRole:
            return QVariant()

        if orientation == Qt.Orientation.Horizontal:
            if section == 0:
                return "Index"
            else:
                return str(self._df.columns[section - 1])

        return QVariant()

    # ----------- Edit Flags -----------
    def flags(self, index) -> Qt.ItemFlags | Qt.ItemFlag:
        if not index.isValid():
            return Qt.ItemFlag.ItemIsEnabled

        # index column is read-only
        if index.column() == 0:
            return Qt.ItemFlag(Qt.ItemFlag.ItemIsSelectable | Qt.ItemFlag.ItemIsEnabled)

        if not self._editable:
            return Qt.ItemFlag(Qt.ItemFlag.ItemIsSelectable | Qt.ItemFlag.ItemIsEnabled)

        return Qt.ItemFlag(Qt.ItemFlag.ItemIsSelectable | Qt.ItemFlag.ItemIsEditable | Qt.ItemFlag.ItemIsEnabled)

    # ----------- Set Data (Write-back to DataFrame) -----------
    def setData(self, index, value, role=Qt.ItemDataRole.EditRole):
        if role != Qt.ItemDataRole.EditRole:
            return False

        if index.column() == 0:
            return False  # index column not editable

        row = index.row()
        col = index.column() - 1  # adjust for index column

        # Update DataFrame
        self._df.iat[row, col] = value
        self._dirty_cells.add((row, index.column()))

        # Emit signal — must include correct topLeft/bottomRight
        self.dataChanged.emit(index, index, [Qt.ItemDataRole.DisplayRole,
                                            Qt.ItemDataRole.BackgroundRole])
        return True



    # ----------- Utility -----------
    def dataframe(self):
        return self._df

    def clear_highlight(self):
        self._dirty_cells.clear()
        top_left = self.index(0, 0)
        bottom_right = self.index(self.rowCount() - 1, self.columnCount() - 1)
        self.dataChanged.emit(top_left, bottom_right, [Qt.ItemDataRole.BackgroundRole])


class GraphEditor(QWidget):
    def __init__(self):
        super().__init__()

        self._loading_tables = False  # Prevents itemChanged loops
        self._changed_items = set()   # Track changed cells for highlight

        self.setWindowTitle("Station Graph Editor")
        self.setGeometry(100, 100, 1100, 600)

        self._main_layout = QHBoxLayout()
        self._left_layout = QVBoxLayout()
        self._main_layout.addLayout(self._left_layout, stretch=1)
        self.setLayout(self._main_layout)

        self.graph_selector = QComboBox()
        self.graph_selector.addItems(stations.keys())
        self.graph_selector.currentIndexChanged.connect(self.load_graph_data)

        self.node_table = QTableView(self)
        self.edge_table = QTableView(self)

        delegate = DataFrameDelegate(self)
        self.node_table.setItemDelegate(delegate)
        self.edge_table.setItemDelegate(delegate)

        self.node_delete_button = QPushButton("Delete Node Row")
        self.node_delete_button.clicked.connect(self.delete_node_row)
        self.save_button = QPushButton("Save", self)
        self.save_button.clicked.connect(self.on_save)
        self.edge_delete_button = QPushButton("Delete Edge Row")
        self.edge_delete_button.clicked.connect(self.delete_edge_row)
        
        # Local copy data
        self.local_station = None

        self.confirm_button = QPushButton("Confirm Changes")
        self.confirm_button.clicked.connect(self.on_confirm)


        # Left side widgets
        self._left_layout.addWidget(self.graph_selector)
        self._left_layout.addWidget(self.node_table)
        self._left_layout.addWidget(self.node_delete_button)
        self._left_layout.addWidget(self.edge_table)
        self._left_layout.addWidget(self.edge_delete_button)
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

        # Create models
        self.node_model = PandasModel(self.local_station.nodes)
        self.edge_model = PandasModel(self.local_station.edges)

        # Connect signals to refresh plot on edit
        self.node_model.dataChanged.connect(self.update_plot)
        self.edge_model.dataChanged.connect(self.update_plot)

        # Install in views
        self.node_table.setModel(self.node_model)
        self.edge_table.setModel(self.edge_model)

        # Set delegates
        delegate = DataFrameDelegate(self)
        self.node_table.setItemDelegate(delegate)
        self.edge_table.setItemDelegate(delegate)

        self.update_plot()




    def update_plot(self):
        self.figure.clear()
        
        if self.local_station is None:
           return
        visualise_station(self.figure, self.local_station ,self.graph_selector.currentText(), label_nodes=True)
        self.canvas.draw()

    def delete_node_row(self):
        if self.local_station is None:
            return

        index = self.node_table.currentIndex()
        row = index.row()
        if row == -1:
            return

        node_index = self.local_station.nodes.index[row]

        # Delete
        self.local_station.nodes = self.local_station.nodes.drop(node_index)
        self.local_station.edges = self.local_station.edges[
            (self.local_station.edges["start"] != node_index) &
            (self.local_station.edges["end"] != node_index)
        ]
        self.local_station.drop_disconnected()

        # Reinstall updated models
        self.node_model = PandasModel(self.local_station.nodes)
        self.node_table.setModel(self.node_model)

        self.edge_model = PandasModel(self.local_station.edges)
        self.edge_table.setModel(self.edge_model)

        self.update_plot()

    def delete_edge_row(self):
        if self.local_station is None:
            return

        index = self.edge_table.currentIndex()
        row = index.row()
        if row == -1:
            return

        # Drop row from the local edges DataFrame
        row_index = self.local_station.edges.index[row]
        self.local_station.edges = self.local_station.edges.drop(row_index)

        # Reinstall updated model
        self.edge_model = PandasModel(self.local_station.edges)
        self.edge_table.setModel(self.edge_model)

        # Keep the same delegate for editing/highlighting
        delegate = DataFrameDelegate(self)
        self.edge_table.setItemDelegate(delegate)

        self.local_station.drop_disconnected()

        # Refresh plot if you want
        self.update_plot()



    def on_item_changed(self, item):
        # Prevent updates triggered by programmatic reloads
        if self._loading_tables:
            return

        if self.local_station is None:
            return

        table = item.tableWidget()

        row = item.row()
        col = item.column()

        # Ignore index column edits since they are read-only anyway
        if col == 0:
            return

        # Determine which dataframe is being edited
        if table is self.node_table:
            df = self.local_station.nodes
        elif table is self.edge_table:
            df = self.local_station.edges
        else:
            return

        # DataFrame col index is offset by 1 (0 is the index column)
        df_col = col - 1
        new_value = item.text()
        df.iat[row, df_col] = new_value

        # Highlight cell to show it was edited
        item.setBackground(Qt.GlobalColor.yellow)

        # Track changes if needed later
        self._changed_items.add((table, row, col))

    def on_confirm(self):
        name = self.graph_selector.currentText()
        stations[name] = copy.deepcopy(self.local_station)

        # Clear highlight
        self.node_model.clear_highlight()
        self.edge_model.clear_highlight()

        # print(f"Changes confirmed for {name}.")


    def on_save(self):
       with open(path[:-4] + "_mod.pkl", "wb") as f:
          pickle.dump(stations, f)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = GraphEditor()
    window.show()
    sys.exit(app.exec_())