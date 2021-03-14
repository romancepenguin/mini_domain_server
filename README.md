# 도메인 서버 개발

대학과제
쓰레드 프로그래밍과 크리티컬 섹션 구현
도메인 캐시 서버 개발

## 서버 기능
- table에 정보가 있으면 도메인 정보 반환.
- table에 정보가 없으면 지정된 마스터 네임서버에 질의 후 결과를 table 파일에 저장
- 질의한 도메인 정보 count ++
- 클라이언트의 접속 기록, 접속 종료 logtable 파일에 기록
- 다중 접속 가능
- table 파일 쓰기 크리티컬 섹션 처리(count 정보를 수정할 때 lock 함)

## 클라이언트 기능
- 도메인 혹은 IP 질의

## 실행 환경
- 리눅스(우분투)

## Usage
서버
```
gcc -pthread server_domain.c -o server
./server [port]
```

클라이언트
```
gcc -pthread client_domain.c -o server
./client [ip] [port]
```

## 실행 이미지(ex.)
서버
![image](https://user-images.githubusercontent.com/28975774/111065110-6325a000-84fb-11eb-82db-50107c69a4a2.png)

클라이언트
![image](https://user-images.githubusercontent.com/28975774/111065144-9405d500-84fb-11eb-853c-6707a0ef48f7.png)
