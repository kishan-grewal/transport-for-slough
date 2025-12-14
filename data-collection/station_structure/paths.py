class StationPath:
  def __init__(self, sequence : list[tuple[int, int]], flow_rows : list[int | tuple[int,float]], reverse_flows: list[int | tuple[int,float]] = []):
    self.sequence = sequence
    self.flow_rows = flow_rows
    self.reverse_flows = reverse_flows

paths = {
  "Bermondsey Underground Station": [
    StationPath([(1000021,3),(1000021,1),(1000021,7),(1000021,9)],[432], [434]),
    StationPath([(1000021,3),(1000021,1),(1000021,7),(1000021,2)],[433], [435]),
  ],
  "Bond Street Underground Station": [
    StationPath([(1000025,6),(1000025,12),(1000025,15),(1000025,4)],[(493,0.5)],[(527,0.5)]), # Entrance1 -> JubNB
    StationPath([(1000025,6),(1000025,12),(1000025,15),(1000025,5)],[(494,0.5)],[(539,0.5)]), # Entrance1 -> JubSB
    StationPath([(1000025,23),(1000025,19),(1000025,17),(1000025,27),(1000025,4)],[(494,0.5)],[(527,0.5)]), # Entrance2 -> JubNB
    StationPath([(1000025,23),(1000025,19),(1000025,17),(1000025,27),(1000025,5)],[(493,0.5)],[(539,0.5)]), # Entrance2 -> JubSB
    StationPath([(1000025,4),(1000025,15),(1000025,2)],[528],[505]), # JubNB -> CenEB
    StationPath([(1000025,5),(1000025,15),(1000025,2)],[535],[506]), # JubSB -> CenEB 
    StationPath([(1000025,4),(1000025,15),(1000025,28)],[529],[517]),# JubNB -> CenWB
    StationPath([(1000025,5),(1000025,15),(1000025,28)],[536],[518]),# JubSB -> CenWB
    StationPath([(1000025,4),(1000025,27),(1000025,17),(1000025,21),(1000025,18)],[531],[554]), # JubNB -> ELEB 
    StationPath([(1000025,5),(1000025,27),(1000025,17),(1000025,21),(1000025,18)],[543],[555]), # JubSB -> ELEB 
    StationPath([(1000025,4),(1000025,27),(1000025,17),(1000025,21),(1000025,25)],[532],[566]), # JubNB -> ELWB 
    StationPath([(1000025,5),(1000025,27),(1000025,17),(1000025,21),(1000025,25)],[544],[567]), # JubSB -> ELWB
  ],
  "Baker Street Underground Station": [
    StationPath([(1000011,12),(1000011,13),(1000011,19),(1000011,7)],[(6,0.5)],[(37,0.5)]), # Entrance1 -> JubNB
    StationPath([(1000011,12),(1000011,13),(1000011,19),(1000011,6)],[(7,0.5)],[(44,0.5)]), # Entrance1 -> JubSB
    
    StationPath([(1000011,16),(1000011,13),(1000011,19),(1000011,7)],[(6,0.5)],[(37,0.5)]), # Entrance2 (Rail) -> JubNB
    StationPath([(1000011,16),(1000011,13),(1000011,19),(1000011,6)],[(7,0.5)],[(44,0.5)]), # Entrance2 (Rail) -> JubSB

    StationPath([(1000011,7),(1000011,19),(1000011,3)],[38],[13]), # JubNB -> BakNB
    StationPath([(1000011,7),(1000011,19),(1000011,2)],[39],[20]), # JubNB -> BakSB
    StationPath([(1000011,6),(1000011,19),(1000011,3)],[45],[14]), # JubSB -> BakNB
    StationPath([(1000011,6),(1000011,19),(1000011,2)],[46],[21]), # JubSB -> BakSB

    StationPath([(1000011,7),(1000011,19),(1000011,9)],[43],[]), # JubNB -> MetE(S)B
    StationPath([(1000011,7),(1000011,19),(1000011,8)],[42],[56]), # JubNB -> MetW(N)B
    StationPath([(1000011,6),(1000011,19),(1000011,9)],[50],[63]), # JubSB -> MetE(S)B
    StationPath([(1000011,6),(1000011,19),(1000011,8)],[49],[57]), # JubSB -> MetW(N)B
    
    StationPath([(1000011,7),(1000011,19),(1000011,13),(1000011,14),(1000011,4)],[],[]), # JubNB -> HamEB
    StationPath([(1000011,7),(1000011,19),(1000011,13),(1000011,14),(1000011,15),(1000011,5)],[],[]), # JubNB -> HamWB

  ],
  "Canning Town Underground Station": [
    StationPath([(1000039,13),(1000039,14),(1000039,1)], [456], [462]),
    StationPath([(1000039,13),(1000039,14),(1000039,15)], [457], [467]),
    ###### REWORK CANNING TOWN TO FLAG SECOND ENTRANCE THE DLR CONNECTION
    StationPath([(1000039,5),(1000039,1)],[473,478,483,487],[463,464,465,466]),
    StationPath([(1000039,5),(1000039,15)],[474,479,488],[468,469,470,471]), # Missing data?
  ],
  "Canons Park Underground Station": [
    StationPath([(1000041,2),(1000041,4),(1000041,3)],[(64,0.5)],[(66,0.5)]),
    StationPath([(1000041,2),(1000041,4),(1000041,6)],[(65,0.5)],[(67,0.5)]),
    StationPath([(1000041,5),(1000041,4),(1000041,3)],[(64,0.5)],[(66,0.5)]),
    StationPath([(1000041,5),(1000041,4),(1000041,6)],[(65,0.5)],[(67,0.5)]),
  ],
  "Canada Water Underground Station": [
    StationPath([(1000037,8),(1000037,5),(1000037,10)],[],[]), # Entrance1 -> JubNB
    StationPath([(1000037,8),(1000037,5),(1000037,1)],[],[]), # Entrance1 -> JubSB
    StationPath([(1000037,9),(1000037,5),(1000037,10)],[],[]), # Entrance2 -> JubNB
    StationPath([(1000037,9),(1000037,5),(1000037,1)],[],[]), # Entrance2 -> JubSB
    StationPath([(1000037,4),(1000037,5),(1000037,10)],[],[]), # Entrance3 -> JubNB
    StationPath([(1000037,4),(1000037,5),(1000037,1)],[],[]), # Entrance3 -> JubSB
  ],
  "Canary Wharf Underground Station":[
    StationPath([(1000038,12),(1000038,2),(1000038,8)],[],[]), # Entrance 1 -> JubN(W)B
    StationPath([(1000038,12),(1000038,2),(1000038,5)],[],[]), # Entrance 1 -> JubS(E)B

    StationPath([(1003008,5),(1000038,1),(1000038,2),(1000038,8)],[],[]), # Entrance 2 (Rail) -> JubN(W)B
    StationPath([(1003008,5),(1000038,1),(1000038,2),(1000038,5)],[],[]), # Entrance 2 (Rail) -> JubS(E)B
    
    StationPath([(1000038,8),(1000038,2),(1000038,1),(1002163,1)],[],[]), # JubN(W)B- -> ELEB
    StationPath([(1000038,5),(1000038,2),(1000038,1),(1002163,5)],[],[]), # JubS(E)B- -> ELWB
  ],
  "Dollis Hill Underground Station": [
    StationPath([(1000061,3),(1000061,5),(1000061,4)],[68],[70]), # Entrance1 -> JubNB
    StationPath([(1000061,3),(1000061,5),(1000061,7)],[69],[71]), # Entrance1 -> JubSB
    StationPath([(1000061,2),(1000061,5),(1000061,4)],[68],[70]), # Entrance2 -> JubNB
    StationPath([(1000061,2),(1000061,5),(1000061,7)],[69],[71]), # Entrance2 -> JubSB
  ],
  "Finchley Road Underground Station": [
    StationPath([(1000082,6),(1000082,7),(1000082,2)],[],[]), # Entrance 1 -> JubNB
    StationPath([(1000082,6),(1000082,7),(1000082,3)],[],[]), # Entrance 1 -> JubSB
    StationPath([(1001109,3),(1000082,7),(1000082,2)],[],[]), # Entrance 2 (Rail) -> JubNB
    StationPath([(1001109,3),(1000082,7),(1000082,3)],[],[]), # Entrance 2 (Rail) -> JubSB

    StationPath([(1000082,2),(1000082,7),(1000082,4)],[],[]), # JubNB -> MetN(W)B
    StationPath([(1000082,3),(1000082,7),(1000082,5)],[],[]),  # JubSB -> MetS(E)B
    
  ],
  "Green Park Underground Station": [
    StationPath([(1000093,5),(1000093,6),(1000093,19),(1000093,3)],[],[]), # Entrance -> JubN(W)B
    StationPath([(1000093,5),(1000093,6),(1000093,19),(1000093,9)],[],[]), # Entrance -> JubS(E)B
    
    StationPath([(1000093,3),(1000093,19),(1000093,6),(1000093,18),(1000093,17),(1000093,1)],[],[]), # JubN(W)B -> PicEB
    StationPath([(1000093,3),(1000093,19),(1000093,6),(1000093,18),(1000093,17),(1000093,15)],[],[]), # JubN(W)B -> PicWB
    StationPath([(1000093,9),(1000093,19),(1000093,6),(1000093,18),(1000093,17),(1000093,1)],[],[]), # JubS(E)B -> PicEB
    StationPath([(1000093,9),(1000093,19),(1000093,6),(1000093,18),(1000093,17),(1000093,15)],[],[]), # JubS(E)B -> PicWB
    
    StationPath([(1000093,3),(1000093,19),(1000093,6),(1000093,18),(1000093,13)],[],[]), # JubN(W)B -> VicEB
    StationPath([(1000093,3),(1000093,19),(1000093,6),(1000093,18),(1000093,7)],[],[]), # JubN(W)B -> VicWB
    StationPath([(1000093,9),(1000093,19),(1000093,6),(1000093,18),(1000093,13)],[],[]), # JubS(E)B -> VicEB
    StationPath([(1000093,9),(1000093,19),(1000093,6),(1000093,18),(1000093,7)],[],[]), # JubS(E)B -> VicWB
  ],
  "Kilburn Underground Station": [
    StationPath([(1000126,2),(1000126,4),(1000126,6)],[139,147,149],[141,143,144]), # Entrance + Rail -> JubNB
    StationPath([(1000126,2),(1000126,4),(1000126,1)],[140,148,150],[142,145,146]), # Entrance + Rail -> JubNB
  ],
  "Kingsbury Underground Station": [
    StationPath([(1000128, 5), (1000128, 4), (1000128, 1)], [151], [153]), # Entrance -> JubNB
    StationPath([(1000128, 5), (1000128, 4), (1000128, 2)], [152], [154]), # Entrance -> JubSB
  ],
  "London Bridge Underground Station": [
    StationPath([(1000139,17),(1000139,16),(1000139,19),(1000139,3),(1000139,27)],[],[]), # Entrance 1 -> JubN(W)B
    StationPath([(1000139,17),(1000139,16),(1000139,19),(1000139,3),(1000139,2)],[],[]), # Entrance 1 -> JubS(E)B
    
    StationPath([(1002056,2),(1000139,4),(1000139,8),(1000139,27)],[],[]), # Entrance 2 (Rail) -> JubN(W)B
    StationPath([(1002056,2),(1000139,4),(1000139,8),(1000139,2)],[],[]), # Entrance 2 (Rail) -> JubS(E)B
    
    StationPath([(1000139,27),(1000139,10),(1000139,9)],[],[]), # JubN(W)B -> NorNB
    StationPath([(1000139,27),(1000139,10),(1000139,27)],[],[]), # JubN(W)B -> NorSB
    StationPath([(1000139,2),(1000139,10),(1000139,9)],[],[]), # JubS(E)B -> NorNB
    StationPath([(1000139,2),(1000139,10),(1000139,27)],[],[]), # JubS(E)B -> NorSB
  ],
  "Neasden Underground Station": [
    StationPath([(1000153,2),(1000153,4),(1000153,3)],[155],[157]), # Entrance -> JubNB
    StationPath([(1000153,2),(1000153,4),(1000153,6)],[156],[158]), # Entrance -> JubNS
  ],
  "North Greenwich Underground Station": [
    StationPath([(1000160, 7),(1000160, 4),(1000160, 2)],[],[]), # Entrance 1 -> JubNB
    StationPath([(1000160, 7),(1000160, 4),(1000160, 1)],[],[]), # Entrance 1 -> JubNB
    StationPath([(1000160, 5),(1000160, 4),(1000160, 2)],[],[]), # Entrance 2 (Rail) -> JubNB
    StationPath([(1000160, 5),(1000160, 4),(1000160, 1)],[],[]), # Entrance 2 (Rail) -> JubNB
  ],
  "Queensbury Underground Station": [
    StationPath([(1000185,2),(1000185,4),(1000185,3)],[159],[161]), # Entrance -> JubNB
    StationPath([(1000185,2),(1000185,4),(1000185,5)],[160],[162]), # Entrance -> JubSB
  ],
  "St. John's Wood Underground Station": [
    StationPath([(1000222,2),(1000222,4),(1000222,3)],[163],[165]), # Entrance -> JubNB
    StationPath([(1000222,2),(1000222,4),(1000222,6)],[164],[166]), # Entrance -> JubSB
  ],
  "Stratford Underground Station": [
    StationPath([(1000226,90),(1000226,70),(1000226,4),(1000226,5),(1000226,8)],[],[]), # Entrance 1 + Rail + DLR -> Jub
    StationPath([(1000226,91),(1000226,4),(1000226,5),(1000226,8)],[],[]), # Entrance 2 -> Jub

    StationPath([(1000226,8),(1000226,5),(1000226,10),(1000226,2)],[],[]), # Jub -> CenE
    StationPath([(1000226,8),(1000226,5),(1000226,10),(1000226,14)],[],[]), # Jub -> ELEB (RPLE)
    
    StationPath([(1000226,8),(1000226,5),(1000226,10),(1000226,92),(1000226,1),(1000226,3)],[],[]), # Jub -> CenW
    StationPath([(1000226,8),(1000226,5),(1000226,10),(1000226,92),(1000226,1),(1000226,15)],[],[]), # Jub -> ELWB (RPLW)

    StationPath([(1000226,8),(1000226,5),(1000226,10),(1000226,92),(1000226,1),(1000226,3)],[],[]), # Jub -> Overground (RPLX)
  ],
  "Stanmore Underground Station": [
    StationPath([(1000219,7),(1000219,2)],[167],[168]) # Entrance -> Jub
  ],
  "Swiss Cottage Underground Station": [
    StationPath([(1000230,4),(1000230,6),(1000230,5)],[],[]), # Entrance 1 -> JubNB
    StationPath([(1000230,4),(1000230,6),(1000230,10)],[],[]), # Entrance 1 -> JubSB

    StationPath([(1002157,0),(1000230,8),(1000230,6),(1000230,5)],[],[]), # Entrance 2 (Rail) -> JubNB
    StationPath([(1002157,0),(1000230,8),(1000230,6),(1000230,10)],[],[]), # Entrance 2 (Rail) -> JubSB
    
    StationPath([(1001260,2),(1000230,2),(1000230,6),(1000230,5)],[],[]), # Entrance 3 (Rail) -> JubNB
    StationPath([(1001260,2),(1000230,2),(1000230,6),(1000230,10)],[],[]), # Entrance 3 (Rail) -> JubSB
  ],
  "Southwark Underground Station": [
    StationPath([(1000215,2),(1000215,1),(1000215,5),(1000215,6),(1000215,9)],[],[]), # Entrance 1 -> JubNB
    StationPath([(1000215,2),(1000215,1),(1000215,5),(1000215,6),(1000215,3)],[],[]), # Entrance 1 -> JubSB
    
    StationPath([(1001313,1),(1000215,4),(1000215,5),(1000215,6),(1000215,9)],[],[]), # Entrance 2 (Rail) -> JubNB
    StationPath([(1001313,1),(1000215,4),(1000215,5),(1000215,6),(1000215,3)],[],[]), # Entrance 2 (Rail) -> JubSB
  ],
  "West Hampstead Underground Station": [
    StationPath([(1000263,2),(1000263,4),(1000263,5)],[],[]), # Entrance + Rail -> JubNB
    StationPath([(1000263,2),(1000263,4),(1000263,3)],[],[]), # Entrance + Rail -> JubSB
  ],
  "Willesden Green Underground Station": [
    StationPath([(1000270,4),(1000270,3)],[428],[430]), # Entrance -> JubNB
    StationPath([(1000270,4),(1000270,9)],[429],[431]), # Entrance -> JubSB
  ],
  "Waterloo Underground Station": [
    StationPath([(1002080, 2),(1000254, 7),(1000254,10),(1000254,12),(1000254,19),(1000254,25)],[],[]), # Entrance 1 -> JubN(W)B
    StationPath([(1002080, 2),(1000254, 7),(1000254,10),(1000254,12),(1000254,19),(1000254,2)],[],[]), # Entrance 1 -> JubS(E)B
    
    StationPath([(1001313,6),(1000254, 13),(1000254, 1),(1000254,25)],[],[]), # Entrance 2 (Rail) -> JubN(W)B
    StationPath([(1001313,6),(1000254, 13),(1000254, 1),(1000254,2)],[],[]), # Entrance 2 (Rail) -> JubS(E)B
    
    StationPath([(1000254,25),(1000254, 19),(1000254, 12),(1000254,5)],[],[]), # JubN(W)B -> NorNB
    StationPath([(1000254,25),(1000254, 19),(1000254, 12),(1000254,24)],[],[]), # JubN(W)B -> NorSB
    StationPath([(1000254,2),(1000254, 19),(1000254, 12),(1000254,5)],[],[]), # JubN(W)B -> NorNB
    StationPath([(1000254,2),(1000254, 19),(1000254, 12),(1000254,24)],[],[]), # JubS(E)B -> NorSB
    
    StationPath([(1000254,25),(1000254, 19),(1000254, 12),(1000254,8)],[],[]), # JubN(W)B -> BakNB
    StationPath([(1000254,25),(1000254, 19),(1000254, 12),(1000254,3)],[],[]), # JubN(W)B -> BakSB
    StationPath([(1000254,2),(1000254, 19),(1000254, 12),(1000254,8)],[],[]), # JubN(W)B -> BakNB
    StationPath([(1000254,2),(1000254, 19),(1000254, 12),(1000254,3)],[],[]), # JubS(E)B -> BakSB
    
    StationPath([(1000254,25),(1000254, 19),(1000254, 12),(1000254,18),(1000254,20)],[],[]), # JubN(W)B -> WCArr
    StationPath([(1000254,25),(1000254, 19),(1000254, 12),(1000254,18),(1000254,9)],[],[]), # JubN(W)B -> WCDep
    StationPath([(1000254,2),(1000254, 19),(1000254, 12),(1000254,18),(1000254,20)],[],[]), # JubN(W)B -> WCArr
    StationPath([(1000254,2),(1000254, 19),(1000254, 12),(1000254,18),(1000254,9)],[],[]), # JubS(E)B -> WCDep
  ],
  "Westminster Underground Station": [
    StationPath([(1000266,10),(1000266,4),(1000266,12),(1000266,11)],[],[]), # Entrance 1 -> JubN(W)B
    StationPath([(1000266,10),(1000266,4),(1000266,12),(1000266,6)],[],[]), # Entrance 1 -> JubS(E)B
    
    StationPath([(1002085,2),(1000266,8),(1000266,4),(1000266,12),(1000266,11)],[],[]), # Entrance 2 (Rail) -> JubN(W)B
    StationPath([(1002085,2),(1000266,8),(1000266,4),(1000266,12),(1000266,6)],[],[]), # Entrance 2 (Rail) -> JubS(E)B

    StationPath([(1000266,11),(1000266,12),(1000266,5)],[],[]), # JubN(W)B -> DisWB
    StationPath([(1000266,11),(1000266,12),(1000266,3)],[],[]), # JubN(W)B -> DisEB
    StationPath([(1000266,6),(1000266,12),(1000266,5)],[],[]), # JubS(E)B -> DisWB
    StationPath([(1000266,6),(1000266,12),(1000266,3)],[],[]), # JubS(E)B -> DisEB
  ],
  "Wembley Park Underground Station": [
    StationPath([(1000257,6),(1000257,8),(1000257,2)], [], []), # Entrance 1 -> Jub NB
    StationPath([(1000257,6),(1000257,8),(1000257,3)], [], []), # Entrance 1 -> Jub SB
    
    StationPath([(1000257,9),(1000257,8),(1000257,2)], [], []), # Entrance 2 -> Jub NB
    StationPath([(1000257,9),(1000257,8),(1000257,3)], [], []), # Entrance 2 -> Jub SB
    
    StationPath([(1000257,2),(1000257,8),(1000257,12)], [], []), # Jub NB -> Met2
    StationPath([(1000257,2),(1000257,8),(1000257,7)], [], []), # Jub NB -> Met5
    StationPath([(1000257,2),(1000257,8),(1000257,4)], [], []), # Jub NB -> Met6
    StationPath([(1000257,3),(1000257,8),(1000257,12)], [], []), # Jub SB -> Met2
    StationPath([(1000257,3),(1000257,8),(1000257,7)], [], []), # Jub SB -> Met5
    StationPath([(1000257,3),(1000257,8),(1000257,4)], [], []), # Jub SB -> Met6
  ]
}