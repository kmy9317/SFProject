# 🎮 Soul Forged (단련된 영혼)

> **Unreal Engine 5 GAS 기반의 3인칭 로그라이크 멀티플레이 ARPG**

`Soul Forged`는 언리얼 엔진의 **GAS(Gameplay Ability System)**를 코어 아키텍처로 채택하여 개발된 멀티플레이 ARPG입니다. 모듈화된 프레임워크, 네트워크 최적화, 그리고 데이터 주도형(Data-Driven) 설계를 통해 확장성 있는 게임 구조를 지향합니다.

<p align="center">
  <img src="https://github.com/user-attachments/assets/6e01bcef-70c7-4707-b62b-33e11b84ff4d" alt="Soul Forged" />
</p>

---

## 📑 목차 (Table of Contents)

1. [프로젝트 소개 & 시연](#-프로젝트-소개--시연)
2. [담당 파트 (김민영, 본인)](#-담당-파트-김민영-본인)
3. [개발 환경](#-개발-환경)
4. [서버 및 데이터 아키텍처](#-서버-및-데이터-아키텍처)
5. [캐릭터 및 로코모션](#-캐릭터-및-로코모션-locomotion)
6. [카메라 및 타겟팅 시스템](#-카메라-및-타겟팅-시스템)
7. [전투 및 사망 아키텍처](#-전투-및-사망-아키텍처)
8. [AI 및 몬스터 시스템](#-ai-및-몬스터-시스템)
9. [인벤토리 및 스킬 시스템](#-인벤토리-및-스킬-시스템)
10. [초기화 및 스테이지 관리](#-초기화-및-스테이지-관리)
11. [성능 최적화](#-성능-최적화)
12. [팀원 및 역할](#-팀원-및-역할-team--roles)

---

## 📽 프로젝트 소개 & 시연

| 항목          | 내용                                                                                  |
| :------------ | :------------------------------------------------------------------------------------ |
| **장르**      | 3인칭 로그라이크 멀티플레이 ARPG (소울라이크 액션)                                    |
| **개발 기간** | 2025.11.03 ~ 2026.01.14 (약 10주)                                                     |
| **개발 규모** | 7인 팀 (프로그래밍 6 / 레벨 디자인 1)                                                 |
| **플랫폼**    | PC (Steam)                                                                            |
| **주요 특징** | 멀티플레이 동기화 · 보스 패턴 AI · GAS 기반 전투 · 로그라이크 강화/진화 시스템 |

|                                                                   Demo Video                                                                   |                                                                   Cinematic                                                                    |                                                                          Presentation                                                                           |
| :--------------------------------------------------------------------------------------------------------------------------------------------: | :--------------------------------------------------------------------------------------------------------------------------------------------: | :-------------------------------------------------------------------------------------------------------------------------------------------------------------: |
| [![Demo](https://github.com/user-attachments/assets/29ce31fd-15bc-476f-9364-9f841f99bae0)](https://www.youtube.com/watch?v=5vq2L-Wd2ok&t=302s) | [![Ending](https://github.com/user-attachments/assets/29ce31fd-15bc-476f-9364-9f841f99bae0)](https://www.youtube.com/watch?v=zWCPcy0zPuE&t=6s) | [![PPT](https://github.com/user-attachments/assets/29ce31fd-15bc-476f-9364-9f841f99bae0)](https://www.canva.com/design/DAG9z8Sl1V4/CFxDLYju_2sQO6aJiEy08w/edit) |

> 📖 **상세 트러블슈팅·기술적 의사결정 과정**은 위 [Presentation 자료](https://www.canva.com/design/DAG9z8Sl1V4/CFxDLYju_2sQO6aJiEy08w/edit)에서 확인하실 수 있습니다.

---

## 👤 담당 파트 (김민영, 본인)

> **Gameplay Systems Programmer** — 게임플레이 시스템 아키텍처 및 게임 전반의 데이터 흐름·UI 연동 설계 담당

| 카테고리                  | 담당 내용                                                              | 본 README 관련 섹션                                                                             |
| ------------------------- | ---------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------- |
| **플레이어 라이프사이클** | 로비 ↔ 인게임 플레이어 초기화, 사망/관전/부활 시스템, 로딩 스크린 설계 | [💀 전투 및 사망](#-전투-및-사망-아키텍처) · [🏗 초기화 및 스테이지](#-초기화-및-스테이지-관리) |
| **인벤토리 / 강화**       | 아이템 시스템, 일반 강화, 진화(Skill Upgrade) 시스템                   | [🎒 인벤토리 및 스킬](#-인벤토리-및-스킬-시스템)                                                |
| **스테이지 / 게임 흐름**  | 스테이지 정보 관리, 적 정보 관리, 플레이어별 인게임 정보 관리          | [🏗 초기화 및 스테이지](#-초기화-및-스테이지-관리)                                              |
| **GAS 스킬 프레임워크**   | GAS 기반 팔라딘/소서러 스킬 프레임워크 및 상호작용 시스템 설계         | [🎒 인벤토리 및 스킬](#-인벤토리-및-스킬-시스템)                                                |
| **네트워크 동기화**       | Motion Warping 동기화 (이동 예측 데이터 확장)                          | [🏃 캐릭터 및 로코모션](#-캐릭터-및-로코모션-locomotion)                                        |
| **성능 최적화**           | 스킬 액터 오브젝트 풀링, 매 프레임 부하 분산 (Tick → Timer)            | [⚡ 성능 최적화](#-성능-최적화)                                                                  |

---

## 🛠 개발 환경

- **Engine**: Unreal Engine 5.6.1 (C++ / Blueprint)
- **IDE**: Rider 2025.1.4
- **Backend**: OnlineSubsystem Steam & PlayFab
- **Collaboration**: GitHub LFS, Notion, Slack

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

- **Predictive Network Movement**: `SFHeroMovementComponent`를 확장하여 서버-클라이언트 간 이동 예측(Prediction) 오차 최소화.
- **State-Driven Gait Control**: Gait 열거형 변수를 통한 상태 머신 설계.
- **Unidirectional Transition**: 관성을 고려하여 Sprint 중 즉각적인 Walk 전환을 방지하는 물리적 제약 로직 구현.
- **Seamless Crouch**: 어떤 이동 상태에서도 즉각적인 웅크리기 전환 및 캡슐 컴포넌트 크기 동기화.
- **Motion Warping Sync**: CMC(CharacterMovementComponent) 이동 패킷에 Warp 타겟 데이터를 포함시켜 재시뮬레이션 시에도 공격 방향 보존.

---

## 🎥 카메라 및 타겟팅 시스템

### 1) Context-Aware Camera System

**"스택 기반의 동적 카메라 연출"**

- **Camera Mode Stack & Blending**: 상황(전투, 질주 등)에 따라 카메라 모드를 스택에 Push/Pop 하여 자동 보간 계산.
- **GAS Integration**: `Gameplay Tag`를 트리거로 사용하여 카메라 연출과 로직의 결합도 최소화.
- **Smart Penetration Avoidance**: `SFPenetrationAvoidanceFeeler`를 통한 지형지물 충돌 감지 및 회피.

### 2) Lock-On Architecture

**"의도를 파악하는 하이브리드 타겟팅"**

- **Hybrid Targeting Algorithm**: `(Distance Score * α) + (Angle Score * β)` 공식을 활용하여 화면 중앙 가중치 기반 타겟 선정.
- **Occlusion Grace Period**: 시야 가림 발생 시 즉시 해제하지 않고 유예 시간을 두어 전투 연속성 보장.
- **Interface-Driven Modularity**: `ISFLockOnInterface`를 통해 몬스터, 오브젝트 등 클래스에 구애받지 않는 범용 타겟팅 구현.
- **Multi-Socket Targeting**: 보스(Dragon)의 경우 머리, 다리 등 다중 소켓 타겟팅을 지원하여 부위 파괴 메커니즘 토대 마련.

---

## 💀 전투 및 사망 아키텍처

### Tactical Death & Spectator

**"죽음 이후의 경험까지 설계된 관전 시스템"**

- **Decoupled Death Flow**: `AttributeSet`의 체력 고갈 시 델리게이트를 통해 어빌리티(`SFGA_Hero_Death`)와 UI를 동시에 호출하는 결합도 낮은 구조.
- **Optimized Spectator Networking**:
  - **Bandwidth Efficiency**: 관전자가 존재할 때만 `Death Spectate Component` 활성화.
  - **Unreliable RPC Strategy**: 30Hz 주기의 Unreliable RPC로 카메라 데이터를 전송하여 실시간성 확보.
  - **Dead Reckoning**: 낮은 업데이트 주기에서도 클라이언트측 선형 보간을 통해 부드러운 관전 화면 구현.
- **Revive & Last Stand**: 다운 상태 → 팀원 상호작용 부활 → 사망 → 관전의 4단계 라이프사이클을 GameplayTag·GameplayCue 중심으로 분리 설계.

---

## 👹 AI 및 몬스터 시스템

### 1) 몬스터 아키텍처

- **통합 프레임워크**: `ASFCharacterBase`를 상속받는 일반 몬스터와 보스의 공통 전투 로직 구축.
- **Data-Driven 설계**: `USFEnemyData` 에셋을 통해 BT, 상태 머신, 몽타주, 스탯을 코드 수정 없이 관리.

### 2) 지능형 행동 제어

- **Hybrid AI**: `AIPerception`, Threat 등 몬스터별로 각기 다른 타겟팅 시스템 구현. `USFEnemyCombatComponent`를 통해 거리/각도 기반 최적 어빌리티 선택(`SelectAbility`).
- **Dragon Movement**: `Grounded`, `Diving`, `Hovering` 등을 포함한 상태 머신 구현.
- **GAS 기반 공격**: 모든 패턴을 `GameplayAbility`로 구현하고 `CalcAIScore`를 통해 AI가 최적의 패턴을 선택하도록 설계.

---

## 🎒 인벤토리 및 스킬 시스템

### 1) 아이템 및 인벤토리

- **Definition/Instance 분리**: `USFItemDefinition`(불변 데이터)과 `USFItemInstance`(런타임 데이터)를 분리하여 메모리 사용량 최적화.
- **Fragment 패턴**: 아이템 기능(소비, 자동획득, 장비, 스탯변경)을 조립식 Fragment로 정의하여 신규 아이템 추가 시 코드 수정 최소화.
- **FastArraySerializer**: 네트워크 복제 시 변경된 슬롯 데이터만 전송하여 대역폭 최적화.
- **Quickbar Component**: PlayerController 단위 4슬롯 빠른 사용 시스템.

### 2) 강화 / 진화 (Roguelike Progression)

- **일반 강화 (Common Upgrade)**: 스테이지 클리어 보상으로 획득한 자원을 소모하여 스탯 상승.
- **스킬 진화 (Skill Evolution)**: 보스 클리어 후 3장 카드 선택 UI를 통한 스킬 분기 — 동일 스킬이 속성/패턴별로 분화.
- **컨텍스트 분리 설계**: 강화·진화 로직을 별도 컴포넌트로 격리하여 PlayerState 비대화 방지.

### 3) 히어로 스킬 프레임워크 (GAS 기반 근접 스킬)

- **타입별 베이스 클래스**: Thrust / Combo / Parry 계열 등 근접 스킬 카테고리별 베이스 어빌리티를 구축하여 신규 스킬 추가 시 보일러플레이트 최소화.
- **Combo System**: 연속 입력을 통한 콤보 공격 및 GAS 기반의 쿨다운/코스트 관리.
- **Motion Warping**: Windup 구간 동안 타겟을 추적하여 조작감 개선.

---

## 🏗 초기화 및 스테이지 관리

### 1) In-Game Initialization Flow

- **3단계 순차 초기화**:
  1. **Data Injection**: 서버에서 `PawnData` 비동기 로드 및 주입.
  2. **State Propagation**: `InitState` 기반 상태 변경 감지 및 전파.
  3. **Function Init**: ASC 초기화 및 AI/상태 머신 활성화.
- **Seamless Travel 데이터 보존**: `CopyProperties()`와 `SavedASCData`를 통해 스테이지 이동 시 스탯/아이템 데이터 유지.

### 2) 스테이지 / 적 정보 관리

- **GameState Manager 패턴**: `SFStageManagerComponent`(현재 스테이지·보스 추적·플레이어 수 기반 적 스케일링), `SFEnemyManagerComponent`(스폰 추적·전멸 감지), `SFPortalManagerComponent`(포탈 활성화)로 분리 설계.
- **플레이어별 인게임 정보 관리**: `PlayerState` 단위 통계·강화 컴포넌트 격리 (`SFPermanentUpgradeComponent`, `SFCommonUpgradeComponent`, `SFPlayerStatsComponent`).

### 3) 로딩 시스템

- **Hybrid Loading Screen**: `MoviePlayer`(Hard Travel)와 `CommonLoadingScreen`(Seamless Travel)을 아우르는 하이브리드 로딩 시스템.
- **번들 로드 통합**: 트래블 직전 에셋 번들을 사전 로드하여 인게임 진입 후 스파이크 제거.
- **DataTable 기반 맵별 로딩 위젯**: `FSFMapLoadingConfig`로 맵에 따라 로딩 화면 위젯을 동적 결정.

---

## ⚡ 성능 최적화

### 1) Object Pooling for Skill Actors

**"멀티플레이 환경에서의 스킬 액터 풀링 최적화"**

- **Deferred Spawn + Replication Suspension**: 예열 액터를 일반 스폰으로 만들면 기본 객체의 복제 플래그가 켜진 상태라 네트워크 등록이 완료되어 불필요한 클라이언트 복제가 발생. 지연 스폰 + 복제 비활성으로 예열 단계 네트워크 등록 차단, 첫 활성화 이후에만 복제 활성화.
- **Dormancy-Based Recycling**: 반납 시 액터를 파괴하지 않고 복제 휴면 상태로 전환해 인스턴스를 유지한 채 복제 트래픽만 차단. 재사용 시 휴면 해제로 기존 인스턴스 그대로 활용.
- **Result**: 스킬 액터 준비 비용 **58% 감소** (1.2ms → 0.5ms, Unreal Insights 측정)

### 2) Frame Load Distribution

**"매 프레임 부하 분산"**

- **Tick → Timer Migration**: Reticle 위치 갱신, 장판 끌어들이기 판정 등 매 프레임 Tick 로직을 일정 간격 Timer로 교체.
- **Spike Avoidance**: 엔진 내부 무거운 작업이 집중된 프레임에도 반드시 부하가 더해지던 구조를 호출 간격 설정으로 회피.

---

## 👨‍💻 팀원 및 역할 (Team & Roles)

| 이름                | 역할                        | 담당 파트                                                                                                                                                                                           |
| ------------------- | --------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **박준범**          | Lead AI Programmer          | Boss AI 아키텍처, GAS 전투 시스템 설계                                                                                                                                                              |
| **안지호**          | AI Programmer               | Enemy Grunt / Enemy Elite AI 제작                                                                                                                                                                   |
| **곽준상**          | Level Designer              | 레벨 디자인 구조 설계                                                                                                                                                                               |
| **김민영**  | Gameplay Systems Programmer | 로비↔인게임 플레이어 초기화, 인벤토리/아이템, 일반 강화·진화 시스템, GAS 기반 팔라딘/소서러 스킬 프레임워크 및 상호작용 시스템 설계, Motion Warping 동기화, 오브젝트 풀링·매 프레임 부하 분산, 스테이지/적/플레이어별 인게임 정보 관리, 사망/관전/부활/로딩스크린 |
| **최윤호**          | Combat & Camera Programmer  | 3인칭 카메라 & 하이브리드 락온, 캐릭터 로코모션(회피/전력질주)                                                                          |
| **이정국**          | UI/UX Programmer            | InGame/OutGame UI 개발 및 데이터 연동                                                                                                                                                               |
| **허중영**          | Online & Backend Programmer | OSS 연동 및 메인메뉴↔로비 접속 흐름 구현, PlayFab 저장/복구, 소서러/팔라딘 스킬 서브 개발                                                                                                           |

---
