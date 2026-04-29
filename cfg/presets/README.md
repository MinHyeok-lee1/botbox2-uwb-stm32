# Runtime Presets

이 디렉터리는 runtime preset sidecar manifest를 보관한다. 여기의 파일은 펌웨어 바이너리 `cfg.bin`을 대체하지 않으며, 현재 단계에서는 운영 정책과 검증 상태를 사람이 검토할 수 있게 정리하는 문서형 manifest다.

## 현재 항목

- `SAFE_DS_1T3A_6HZ_BASELINE.json`: 기존 6 Hz / N=3 운영 베이스라인을 SAFE profile로 기록한 초기 항목.
- `TEMPLATE_CANDIDATE_RUNTIME_PRESET.json`: 신규 CANDIDATE sidecar manifest 작성용 template. 실제 검증 완료 preset이 아니다.

## Template 사용법

- template을 복사해 새 preset JSON을 만들 때는 `preset_id`, `topology`, `runtime_profile`, `evidence`, `validation`, `approval` 값을 실제 검토 범위에 맞게 채운다.
- `status`는 SAFE로 시작하지 않는다. SAFE 승격은 `docs/antigravity/plans/2026-04-28-runtime-preset-promotion-criteria.md` 기준으로 사람 승인과 검증 기록이 필요하다.
- 이 template은 `cfg.bin` 생성물도, `delivery/configs/*.bin` 대체물도, firmware parser 구현도 아니다.
- 자동 SAFE 승격을 암시하지 않는다.

## 유지 규칙

- SAFE 항목은 검증된 보수적 운영 기준만 담는다.
- CANDIDATE 항목은 기본 활성화나 SAFE 승격을 의미하지 않는다.
- `DS_BOUNDED_SCHEDULER`와 `DS_ANCHOR_OBS_SUMMARY`는 명시적 승인 없이 SAFE 기본값으로 활성화하지 않는다.
- binary `cfg` 형식 변경은 별도 승인 전까지 이 디렉터리의 범위가 아니다.
