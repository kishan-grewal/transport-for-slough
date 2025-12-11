class StationPath:
  def __init__(self, sequence : list[tuple[int, int]], flow_rows : list[int], reverse_flows: list[int] | None = None):
    self.sequence = sequence
    self.flow_rows = flow_rows
    self.reverse_flows = reverse_flows

paths = {
  "Kingsbury Underground Station": [
    StationPath([(1000128, 5), (1000128, 4), (1000128, 1)], [151], [153]), # Entrance -> JubNB
    StationPath([(1000128, 5), (1000128, 4), (1000128, 2)], [152], [154]), # Entrance -> JubSB
  ]
}