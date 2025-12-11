class StationPath:
  def __init__(self, sequence : list[tuple[int, int]], flow_rows : list[int], reverse_flows: list[int] = []):
    self.sequence = sequence
    self.flow_rows = flow_rows
    self.reverse_flows = reverse_flows

paths = {
  "Kingsbury Underground Station": [
    StationPath([(1000128, 5), (1000128, 4), (1000128, 1)], [151], [153]), # Entrance -> JubNB
    StationPath([(1000128, 5), (1000128, 4), (1000128, 2)], [152], [154]), # Entrance -> JubSB
  ],
  "Wembley Park Underground Station": [
    StationPath([(1000257,6),(1000257,8),(1000257,2)], [1], [2]),
    StationPath([(1000257,6),(1000257,8),(1000257,3)], [3], [4]),
    StationPath([(1000257,6),(1000257,8),(1000257,7)], [5], [6]),
    StationPath([(1000257,2),(1000257,8),(1000257,7)], [7], [8]),
    StationPath([(1000257,3),(1000257,8),(1000257,7)], [9], [10]),
  ]
}