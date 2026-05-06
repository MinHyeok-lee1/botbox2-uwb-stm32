# UWB_CAN_PACKET_CONTRACT.md

> [!IMPORTANT]
> **로컬 사본 유지 안내**: 제품 프로젝트는 메인보드 통합의 자립성을 유지하기 위해 본 CAN 계약서를 로컬에 직접 유지합니다. 
> 업스트림의 정식 기술 계약본은 내부 관리 가이드를 참조하십시오.

본 문서는 UWB 모듈과 메인보드 간의 CAN 통신 프로토콜 계약을 정의합니다.

## CAN ID 매핑 (CAN ID Mapping)
앵커의 AID(Anchor ID)에 따라 다음과 같이 CAN Standard ID가 할당됩니다:
- **Anchor AID 0x2001** -> CAN Standard ID **0x201**
- **Anchor AID 0x2002** -> CAN Standard ID **0x202**
- **Anchor AID 0x2003** -> CAN Standard ID **0x203**

## 페이로드 형식 (Payload Format)

```c
union UwbPacket {
  struct __attribute__((__packed__)) {
    uint8_t start;      // 시작 바이트 (0xAA)
    int16_t distance;   // 거리 데이터 (Little-endian, mm 단위)
    uint8_t checksum;   // 체크섬 (XOR 연산)
  };
  uint8_t byteStream[4];
};
```

- **DLC**: 4 bytes
- **Byte 0**: `0xAA` (Fixed Start Byte)
- **Byte 1**: `distance` LSB (밀리미터 단위, `int16_t`)
- **Byte 2**: `distance` MSB (밀리미터 단위, `int16_t`)
- **Byte 3**: `checksum` (`byte[0] ^ byte[1] ^ byte[2]`)

## 전송 소유권 (Ownership)
측정 방식에 따라 전송 주체가 달라집니다:
- **SS-TWR**: **TAG**가 CAN 거리 프레임을 전송합니다. 모든 거리가 TAG 측에서 수집 및 계산되기 때문입니다.
- **DS-TWR**: 각 **ANCHOR**가 자신의 CAN 거리 프레임을 전송합니다. 각 앵커가 스스로 거리를 계산하기 때문입니다.

## 계약 테스트 벡터 (Contract Test Vector)
본 벡터는 프로토콜 수준의 인코딩/디코딩 테스트를 위한 것이며, 물리적 측정 성능의 기준이 아닙니다.

- **입력 (Input)**: CAN ID `0x203`, 거리 `744 mm`
- **변환 (Conversion)**:
  - 744 (Decimal) = `0x02E8`
  - `int16` Little-endian = `E8 02`
  - Checksum = `0xAA ^ 0xE8 ^ 0x02` = `0x40`
- **기대되는 CAN 프레임 (Expected CAN frame)**:
  - **ID**: `0x203`
  - **DLC**: `4`
  - **DATA**: `AA E8 02 40`

## 명시적 확인 사항 (State Clearly)
<!-- Keywords: millimeters -->
- CAN ID(`0x201/0x202/0x203`)가 보이는 것만으로는 충분하지 않습니다.
- DATA 페이로드가 UART 로그에 기록된 밀리미터 단위 거리값으로 정확히 디코딩되어야 합니다.
- 본 테스트 벡터는 UWB 레인징의 정확도(Accuracy)를 증명하지 않습니다. 오직 프로토콜 준수 여부만을 확인합니다.
