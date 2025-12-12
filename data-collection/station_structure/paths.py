class StationPath:
  def __init__(self, sequence : list[tuple[int, int]], flow_rows : list[int], reverse_flows: list[int] = []):
    self.sequence = sequence
    self.flow_rows = flow_rows
    self.reverse_flows = reverse_flows

paths = {
  "Bermondsey Underground Station": [
    StationPath([(1000021,3),(1000021,1),(1000021,7),(1000021,9)],[432], [434]),
    StationPath([(1000021,3),(1000021,1),(1000021,7),(1000021,2)],[433], [435]),
  ],
  "Canning Town Underground Station": [
    StationPath([(1000039,13),(1000039,14),(1000039,1)], [456], [462]),
    StationPath([(1000039,13),(1000039,14),(1000039,15)], [457], [467]),
    ###### REWORK CANNING TOWN TO FLAG SECOND ENTRANCE THE DLR CONNECTION
    # StationPath([(1000039,5),(1000039,1)],[473,478,483,487],[463,464,465,466]),
    # StationPath([(1000039,5),(1000039,15)],[474,479,488],[468,469,470,471]), # Missing data?
  ],
  "Canons Park Underground Station": [
    StationPath([(1000041,2),(1000041,4),(1000041,3)],[64],[66]),
    StationPath([(1000041,2),(1000041,4),(1000041,6)],[65],[67]),
    StationPath([(1000041,5),(1000041,4),(1000041,3)],[64],[66]),
    StationPath([(1000041,5),(1000041,4),(1000041,6)],[65],[67]),
  ],
  "Dollis Hill Underground Station": [
    StationPath([(1000061,3),(1000061,5),(1000061,4)],[68],[70]), # Entrance1 -> JubNB
    StationPath([(1000061,3),(1000061,5),(1000061,7)],[69],[71]), # Entrance1 -> JubSB
    StationPath([(1000061,3),(1000061,5),(1000061,4)],[68],[70]), # Entrance2 -> JubNB
    StationPath([(1000061,3),(1000061,5),(1000061,7)],[69],[71]), # Entrance2 -> JubSB
  ],
  "Kilburn Underground Station": [
    StationPath([(1000126,2),(1000126,4),(1000126,6)],[139,147,149],[141,143,144]), # Entrance + Rail -> JubNB
    StationPath([(1000126,2),(1000126,4),(1000126,1)],[140,148,150],[142,145,146]), # Entrance + Rail -> JubNB
  ],
  "Kingsbury Underground Station": [
    StationPath([(1000128, 5), (1000128, 4), (1000128, 1)], [151], [153]), # Entrance -> JubNB
    StationPath([(1000128, 5), (1000128, 4), (1000128, 2)], [152], [154]), # Entrance -> JubSB
  ],
  "Neasden Underground Station": [
    StationPath([(1000153,2),(1000153,4),(1000153,3)],[155],[157]), # Entrance -> JubNB
    StationPath([(1000153,2),(1000153,4),(1000153,6)],[156],[158]), # Entrance -> JubNS
  ],
  "Queensbury Underground Station": [
    StationPath([(1000185,2),(1000185,4),(1000185,3)],[159],[161]), # Entrance -> JubNB
    StationPath([(1000185,2),(1000185,4),(1000185,5)],[160],[162]), # Entrance -> JubSB
  ],
  "St. John's Wood Underground Station": [
    StationPath([(1000222,2),(1000222,4),(1000222,3)],[163],[165]), # Entrance -> JubNB
    StationPath([(1000222,2),(1000222,4),(1000222,6)],[164],[166]), # Entrance -> JubSB
  ],
  "Stanmore Underground Station": [
    StationPath([(1000219,7),(1000219,2)],[167],[168]) # Entrance -> Jub
  ],
  "Willesden Green Underground Station": [
    StationPath([(1000270,4),(1000270,3)],[428],[430]), # Entrance -> JubNB
    StationPath([(1000270,4),(1000270,9)],[429],[431]), # Entrance -> JubSB
  ]
  # "Wembley Park Underground Station": [
  #   StationPath([(1000257,6),(1000257,8),(1000257,2)], [1], [2]),
  #   StationPath([(1000257,6),(1000257,8),(1000257,3)], [3], [4]),
  #   StationPath([(1000257,6),(1000257,8),(1000257,7)], [5], [6]),
  #   StationPath([(1000257,2),(1000257,8),(1000257,7)], [7], [8]),
  #   StationPath([(1000257,3),(1000257,8),(1000257,7)], [9], [10]),
  # ]
}