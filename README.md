# 🎮 Soul Forged (단련된 영혼)
> **Unreal Engine 5 기반의 차세대 3인칭 로그라이크 멀티플레이어 프레임워크**

`Soul Forged`는 언리얼 엔진의 **GAS(Gameplay Ability System)**를 코어 아키텍처로 채택하여 개발된 고성능 멀티플레이어 게임입니다. 단순한 기능 구현을 넘어 모듈화된 프레임워크, 네트워크 최적화, 그리고 데이터 주도형(Data-Driven) 설계를 통해 확장성 있는 게임 구조를 지향합니다.

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

| Demo Video | Ending Cinematic | Presentation |
| :---: | :---: | :---: |
| [![Demo](https://img.youtube.com/vi/mScI6zura8E/0.jpg)](https://youtu.be/mScI6zura8E) | [![Ending](https://img.youtube.com/vi/Rgv1000JS2A/0.jpg)](https://youtu.be/Rgv1000JS2A) | [![PPT](https://img.youtube.com/vi/utTe070kYgI/0.jpg)](https://youtu.be/utTe070kYgI) |

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

* **Predictive Network Movement**: `SFHeroMovementComponent`를 확장하여 서버-클라이언트 간 이동 예측(Prediction) 오차 최소화.
* **State-Driven Gait Control**: Gait 열거형 변수를 통한 상태 머신 설계.
* **Unidirectional Transition**: 관성을 고려하여 Sprint 중 즉각적인 Walk 전환을 방지하는 물리적 제약 로직 구현.
* **Seamless Crouch**: 어떤 이동 상태에서도 즉각적인 웅크리기 전환 및 캡슐 컴포넌트 크기 동기화.
* **Motion Warping Sync**: CMC(CharacterMovementComponent) 이동 패킷에 Warp 타겟 데이터를 포함시켜 재시뮬레이션 시에도 공격 방향 보존.



---

## 🎥 카메라 및 타겟팅 시스템

### 1) Context-Aware Camera System
**"스택 기반의 동적 카메라 연출"**
* **Camera Mode Stack & Blending**: 상황(전투, 질주 등)에 따라 카메라 모드를 스택에 Push/Pop 하여 자동 보간 계산.
* **GAS Integration**: `Gameplay Tag`를 트리거로 사용하여 카메라 연출과 로직의 결합도 최소화.
* **Smart Penetration Avoidance**: `SFPenetrationAvoidanceFeeler`를 통한 지형지물 충돌 감지 및 회피.

### 2) Lock-On Architecture
**"의도를 파악하는 하이브리드 타겟팅"**
* **Hybrid Targeting Algorithm**: `(Distance Score * α) + (Angle Score * β)` 공식을 활용하여 화면 중앙 가중치 기반 타겟 선정.
* **Occlusion Grace Period**: 시야 가림 발생 시 즉시 해제하지 않고 유예 시간을 두어 전투 연속성 보장.
* **Interface-Driven Modularity**: `ISFLockOnInterface`를 통해 몬스터, 오브젝트 등 클래스에 구애받지 않는 범용 타겟팅 구현.
* **Multi-Socket Targeting**: 보스(Dragon)의 경우 머리, 다리 등 다중 소켓 타겟팅을 지원하여 부위 파괴 메커니즘 토대 마련.



---

## 💀 전투 및 사망 아키텍처

### Tactical Death & Spectator
**"죽음 이후의 경험까지 설계된 고성능 관전 시스템"**
* **Decoupled Death Flow**: `AttributeSet`의 체력 고갈 시 델리게이트를 통해 어빌리티(`SFGA_Hero_Death`)와 UI를 동시에 호출하는 응집도 높은 구조.
* **Optimized Spectator Networking**:
    * **Bandwidth Efficiency**: 관전자가 존재할 때만 `Death Spectate Component` 활성화.
    * **Unreliable RPC Strategy**: 30hz 주기의 Unreliable RPC로 카메라 데이터를 전송하여 실시간성 확보.
    * **Dead Reckoning**: 낮은 업데이트 주기에서도 클라이언트측 선형 보간을 통해 부드러운 관전 화면 구현.



---

## 👹 AI 및 몬스터 시스템

### 1) 몬스터 아키텍처
* **통합 프레임워크**: `ASFCharacterBase`를 상속받는 일반 몬스터와 보스의 공통 전투 로직 구축.
* **Data-Driven 설계**: `USFEnemyData` 에셋을 통해 BT, 상태 머신, 몽타주, 스탯을 코드 수정 없이 관리.
### 2) 지능형 행동 제어
* **Hybrid AI**: `AIPerception`, Threat 등 몬스터별로 각기 다른 타겟팅 시스템구현  + `USFEnemyCombatComponent`를 통한 거리/각도 기반 최적 어빌리티 선택(`SelectAbility`).
* **Dragon Movement**: `Grounded`부터 `Diving`, `Hovering`까지 포함된 비행 상태 머신 구현.
* **GAS 기반 공격**: 모든 패턴을 `GameplayAbility`로 구현하고 `CalcAIScore`를 통해 AI가 최적의 패턴을 선택하도록 설계.



---

## 🎒 인벤토리 및 스킬 시스템

### 1) 아이템 및 인벤토리
* **Definition/Instance 분리**: 불변 데이터와 런타임 데이터 분리로 메모리 사용량 최적화.
* **Fragment 패턴**: 아이템 기능을 조립식으로 정의 (소모, 스택 등).
* **FastArraySerializer**: 네트워크 복제 시 변경된 슬롯 데이터만 전송하여 대역폭 최적화.

### 2) 히어로 스킬 시스템
* **Combo System**: 연속 입력을 통한 콤보 공격 및 GAS 기반의 쿨다운/코스트 관리.
* **Motion Warping**: Windup 구간 동안 타겟을 추적하여 조작감 개선.

---

## 🏗 초기화 및 스테이지 관리

### 1) In-Game Initialization Flow
* **3단계 순차 초기화**:
    1. **Data Injection**: 서버에서 `PawnData` 비동기 로드 및 주입.
    2. **State Propagation**: `InitState` 기반 상태 변경 감지 및 전파.
    3. **Function Init**: ASC 초기화 및 AI/상태 머신 활성화.
* **Seamless Travel 데이터 보존**: `CopyProperties()`와 `SavedASCData`를 통해 스테이지 이동 시 스탯/아이템 데이터 유지.

### 2) 스테이지 및 로딩 시스템
* **Stage Subsystem**: 맵 이름 기반 DataTable 조회 및 에셋 번들(Lobby/InGame) 관리.
* **Loading Screen**: `MoviePlayer`(Hard Travel)와 `CommonLoadingScreen`(Seamless Travel)을 아우르는 하이브리드 로딩 시스템.



---

## 🚑 트러블슈팅 (Troubleshooting)

### 1. Seamless Travel 시 데이터 초기화 문제
* **현상**: 레벨 전이 시 플레이어의 스탯과 인벤토리가 소멸함.
* **원인**: 새로운 레벨로 이동하며 `PlayerState`와 `Character`가 재생성됨.
* **해결**: `CopyProperties()`를 오버라이드하여 기존의 ASC 데이터를 임시 구조체에 저장 후 새로운 액터에 성공적으로 복구함으로써 연속적인 게임플레이 환경 구축.

### 2. 멀티플레이어 Motion Warping 방향 불일치
* **현상**: 서버 보정(Correction) 시 클라이언트의 Warp 타겟이 유실되어 공격 방향이 어색하게 돌아감.
* **해결**: CMC의 이동 패킷 내부에 Warp 관련 정보를 커스텀 플래그로 포함시키고, 재시뮬레이션(Resimulation) 시 이를 우선적으로 복구하는 로직을 통해 해결.

---

## 👨‍💻 팀원 및 역할 (Team & Roles)

| 이름 | 역할 | 담당 파트 |
|---|---|---|
| **박준범** | **Main Developer** | Enemy/Boss AI 아키텍처, GAS 전투 시스템

---

