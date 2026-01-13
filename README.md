# 🎮 Soul Forged (단련된 영혼)
> **Unreal Engine 5 기반의 차세대 3인칭 로그라이크 멀티플레이어 프레임워크**

`Soul Forged`는 언리얼 엔진의 **GAS(Gameplay Ability System)**를 코어 아키텍처로 채택하여 개발된 고성능 멀티플레이어 게임입니다. 단순한 기능 구현을 넘어 모듈화된 프레임워크, 네트워크 최적화, 그리고 데이터 주도형(Data-Driven) 설계를 통해 확장성 있는 게임 구조를 지향합니다.

<p align="center">
  <img src="https://github.com/user-attachments/assets/6e01bcef-70c7-4707-b62b-33e11b84ff4d" alt="Soul Forged" />
</p>


---

## 📑 목차 (Table of Contents)
1. [프로젝트 소개 (Project Introduction)](#-프로젝트-소개)
2. [서버 및 데이터 아키텍처 (Server & Data)](#-서버-및-데이터-아키텍처)
3. [캐릭터 및 로코모션 (Character & Locomotion)](#-캐릭터-및-로코모션)
4. [카메라 및 타겟팅 시스템 (Camera & Targeting)](#-카메라-및-타겟팅-시스템)
5. [전투 및 사망 아키텍처 (Combat & Death)](#-전투-및-사망-아키텍처)
6. [AI 및 몬스터 시스템 (Monster AI System)](#-ai-및-몬스터-시스템)
7. [인벤토리 및 스킬 시스템 (Inventory & Skill)](#-인벤토리-및-스킬-시스템)
8. [초기화 및 스테이지 관리 (Initialization & Stage)](#-초기화-및-스테이지-관리)
9. [트러블슈팅 (Troubleshooting)](#-트러블슈팅)

---

## 📽 프로젝트 소개 & 시연

* **장르**: 3인칭 로그라이크 멀티플레이  
* **주요 특징**: 고성능 멀티플레이 동기화, 보스전 패턴 AI, GAS 기반 전투 시스템  

| Demo Video | Cinematic | Presentation |
| :---: | :---: | :---: |
| [![Demo](https://github.com/user-attachments/assets/29ce31fd-15bc-476f-9364-9f841f99bae0)](https://www.youtube.com/watch?v=5vq2L-Wd2ok&t=302s) | [![Ending](https://github.com/user-attachments/assets/29ce31fd-15bc-476f-9364-9f841f99bae0)](https://www.youtube.com/watch?v=zWCPcy0zPuE&t=6s) | [![PPT](https://github.com/user-attachments/assets/29ce31fd-15bc-476f-9364-9f841f99bae0)](https://www.canva.com/design/DAG9z8Sl1V4/CFxDLYju_2sQO6aJiEy08w/edit) |



---

## 🛠 개발 환경
* **Engine**: Unreal Engine 5.6.1 (C++ / Blueprint)
* **IDE**: Rider 2025.1.4
* **Backend**: OnlineSubsystem Steam & PlayFab
* **Collaboration**: GitHub LFS, Notion, Slack

---

## 🌐 서버 및 데이터 아키텍처

### 1) OnlineSubsystem Steam (매치메이킹)
- **SessionInterface**를 통한 방 생성, 검색, 입장 로직 구현.
- 호스트는 `listen` 서버 로비 오픈, 클라이언트는 `connect string` 기반 `ClientTravel` 처리.
- 모든 세션 결과는 델리게이트 브로드캐스트를 통해 UI와 느슨하게 결합.

### 2) PlayFab (영구 데이터 관리)
- **GameInstanceSubsystem**을 활용해 게임 생명주기와 동기화된 데이터 관리.
- **JSON 직렬화**: 유저 데이터 구조체를 JSON으로 변환하여 PlayFab UserData에 저장/로드.
- **네트워크 보장**: 멀티플레이 환경에서 서버 권한 반영을 위해 `PlayerState` 준비 시점을 기다리는 **재시도 타이머 로직** 적용.

---

## 🏃 캐릭터 및 로코모션 (Locomotion)

### Advanced Locomotion System
**"네트워크 환경을 고려한 예측형 이동 시스템"**

* **Predictiv동
---

