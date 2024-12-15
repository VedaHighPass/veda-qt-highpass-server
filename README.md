# 📷 CCTV를 이용한 하이패스 서버 입니다.

## 📖 소개
하이패스를 이용할 경우 발생할 수 있는 사고 위험을 방지하고, CCTV를 활용하여 하이패스 기능을 대체함으로써 인건비 절감, 사고 예방, 교통 체증 감소를 목표로 하는 프로젝트입니다.

![VEDA_finalProject-service architecture detail2 drawio (2)](https://github.com/user-attachments/assets/cba4a6c2-95f8-4036-9d99-a7af0e6b4171)

### 이 저장소는 Main server 의 저장소 입니다.
## 기능
- VMS Server
- RESTful Web Server
- SQLite DB 연동
- OpenSSL 보안기능

## Qt 클라이언트
- https://github.com/VedaHighPass/veda-qt-highpass-client

## Jetson Camera (RTSP server)
- https://github.com/VedaHighPass/rtsp-hyojin

## 🛠️ 설치 및 실행
### 요구 사항
- Linux
- Qt
```bash
sudo apt install ffmpeg
설치 및 실행
프로젝트 클론
git clone https://github.com/VedaHighPass/veda-qt-highpass-server
cd veda-qt-highpass-server
```
### 빌드 및 실행

# 서버 실행 후 클라이언트 실행
```bash
cd veda-qt-highpass-server/
qmake
make
./qtserver
```
📜 라이선스
이 프로젝트는 GPL (GNU General Public License) 2.0 이상 버전의 라이선스를 따릅니다. 자세한 내용은 LICENCE 파일을 참조하세요.
