# Next Steps - 03_stm32-dw3000-G474RE-customer

최종 갱신: 2026-07-16 (Customer CH9 sync and sanitization periodic check)

## P2

- [x] 최신 CH9 fix 및 정화 문서 유지 여부 scanner로 정기 재확인
      상태: Closed
      결과: 02 Product의 CH9 reset/retry hunk와 apply_uwb_cfg 오류 전파 hunk가 03 Customer에 일치함을 확인했고, 최신 delivery firmware/manifest를 02 기준으로 재수출해 SHA256 일치 확인
      착수조건: 02 Product에서 새 accepted fix/export가 발생하거나 Customer delivery 문서가 변경될 때
      관련: 03 Customer sanitized delivery, CH9 fix propagation, lightweight boundary scanner
      완료조건: 03 scanner `FAIL=0 WARN=0`, Product contract checks 통과, 내부 분석/작업 문서 유입 없음
      다음 점검: 다음 02 critical fix 또는 Customer delivery 문서 변경 시

## 참고 - 보류 유지 (재개 금지)

- A7 128Hz 인증 공백: Customer delivery 기준 작업 아님
- A11 DS_BOUNDED_SCHEDULER: 현재 OFF 상태 유지, 명시적 결정 전 재개 금지
- A8 scanner heuristic: 실제 사고/누락 근거 전까지 코드 개선 불필요, 문서화만 유지
- PC0 USER/ACTION: 현 보드 미라우팅 상태이므로 future board revision 전 재개 금지
