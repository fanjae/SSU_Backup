## 2019-봄학기 리눅스 시스템 프로그래밍
+ Multi-Thread를 이용한 파일 Backup 프로그램
+ 실행 환경
	+ Ubuntu 18.04 LTS
	+ gcc 7.5.0
+ 본 Program은 특정 파일또는 디렉토리를 Backup list로 등록시킬 수 있습니다. 등록된 Backup list를 각 Thread가 실행하여 일정 주기마다 Backup을 진행하도록 구현하였습니다.
+ Score : 96/100
+ 개선 필요 사항 : Option의 순서 상관 없이 실행 되도록 처리 필요.
