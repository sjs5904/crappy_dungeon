COM S 327
Assignment#1.10
Jooseung Song
rlg327.cpp

Added place_lava_water. characters lose 2 hp on lava and lose 2 speed on water.
PC regenerate 2 hp every 5 step. press 'c' to see description of anything in selected cell
Had a segmentation issue associated with io_nearest_visible_monster, but seems to be resolved.


### Change Log

1/20/2018: isSafe, generateRoom 함수 추가. 방 생성 가능.  
1/21/2018: 오타 수정, 실행 속도 개선  
1/22/2018: 간단한 길찾기 알고리즘 구현. struct 어레이 정렬하는 함수 추가.  
1/23/2018: generateCorridor, draw 함수 추가. 길찾기 알고리즘을 이용해서 방 구역간 복도 생성 가능.  
1/23/2018: segmentation fault 에러 해결. 메모리 문제 개선.  
1/29/2018: dungeon struct 추가. map data 삭제. init_dungeon, load_dungeon, render_dungeon 추가.  
1/30/2018: 세이브 로드 기능 추가.  
1/30/2018: 불러오기 도중에 뜨는 segmentation fault 에러 해결.  
2/05/2018: Sheaffer 교수님의 솔루션을 참고. 대부분의 코드 구조를 깔끔하게 개선.  
2/05/2018: PC struct 추가. dungeon struct 수정.  
2/10/2018: 기존 복도를 생성하는 코드를 기반으로 다익스트라 알고리즘 구현.  
2/12/2018: place_room 수정. 무한루프 고침.  
2/12/2018: render_dungeon이 이제 플레이어 캐릭터를 랜더링함.  
2/12/2018: place_PC 추가함.  
2/13/2018: render_distance_map, render_tunnel_distance_map 추가.  
2/19/2018: render_dungeon 수정.  
2/19/2018: 캐릭터가 8방향으로 움직일 수 있게 수정.  
2/26/2018: Ncurses 적용. 텍스트 기반 UI를 추가함. 일부 C파일들을 CPP로 바꿈.  
3/3/2018: PC가 벽을 뚫고 다니지 못하게 수정.  
3/4/2018: 기호들이 무엇을 의미하는지 찾아볼 수 있게 UI를 추가함.  
3/6/2018: 메뉴 기능 추가.  
3/8/2018: 메뉴 기능 수정.  
3/19/2018: rgl327.c을 제외한 대부분의 C파일을 CPP로 바꿈.  
3/19/2018: 전장의 안개 추가 시스템 추가.  
3/20/2018: 텔레포트 모드 추가.  
3/20/2018: seg fault 일부 해결.  
3/27/2018 Monster Description 추가. 텍스트 파일에서 적에대한 설명을 읽어옴.  
4/2/2018: 새 오브젝트 추가할 클래스 추가함.  
4/3/2018: 기호 색상을 변경 가능하게 수정함.  
4/3/2018: can_see, gen_monster 수정함.  
4/3/2018: gen_object 추가함.  
4/9/2018: io_list_inventory, io_list_equipment 추가. 인벤토리, 장비 기능 추가.  
4/9/2018: description 클래스 수정.  
4/9/2018: do_combat 수정.  
4/10/2018: io_look 클래스 추가.  
4/10/2018: pc_addCarry, pc_dropItem, pc_wear, pc_takeOff, pc_inspect, pc_update 추가. PC가 아이템을 들거나 떨어뜨리거나 장착, 해제, 확인 할 수 있음  
4/10/2018: 조작 키 수정.  
4/15/2018: ter_water, ter_lava, place_lava_water 추가.  
4/16/2018: move_character를 실행할때 PC가 ter_water, ter_lava 타일에서 특정 효과를 받게함.  
4/16/2018: move_character를 실행할때 일정 간격으로 캐릭터가 체력을 회복하게 수정.  
4/17/2018: io_nearest_visible_monster와 관련된 seg fault 에러 해결.  
