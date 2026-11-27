# Sandpaper Fairy v3.2 — by Sylredhel

순수 와이드밴드 오디오 경로 + **사이드체인 필터(`OFF / HPF / LPF / BAND`)** + 고속 호흡 크레스트 팩터 디텍터 + 전 모드 공용 크레스트 연동 `GRIT DRIVE`를 갖춘 레벨 독립형 크레스트 컴프레서 VST3입니다.

## v3.2 핵심 업데이트
1. **멀티밴드 스플릿 제거 → 순수 사이드체인 필터 전환 (`SC FILTER` + `SC FREQ` + `SC MONITOR`)**
   - 메인 오디오는 크로스오버 분할 없이 **100% 와이드밴드**로 통과시켜 멀티밴드 컴프 특유의 고역 꺼짐·위상 분리감을 없애고 레이턴시도 줄였습니다.
   - `12 dB/oct` Cytomic TPT SVF를 디텍터 전용으로 배치해 **`SC: OFF (Full)`**, **`SC: HPF (Low-Safe)`**, **`SC: LPF (Air-Safe)`**, **`SC: BAND (Focus)`** 4모드를 지원하며, `AUDITION -> SC MONITOR`로 필터링된 사이드체인 소리를 직접 들으며 주파수를 맞출 수 있습니다.
2. **답답함(95 ms 홀드 누적) 해소:**
   - 피크 디텍터 릴리즈를 `12 ms -> 2.5 ms`로 고속화하고, 마이크로 홀드(`<= 2 ms`)·`AUTO REL`(지속 GR `> 3 dB`에서만 최대 `2.2x`)·기본 `LOOKAHEAD (2 ms)`를 재조정해 스파이크 직후 그루브와 에어가 즉시 열리도록 개선했습니다.
3. **`GRIT DRIVE` 전 모드 개방 (`RIDER` + `VARI-MU` 공용):**
   - 깔끔한 선형 VCA `RIDER` 모드에서도 `GRIT DRIVE`를 `+2 ~ +6 dB` 살짝 섞어 스파이크 끝단의 선명도와 엔벨로프를 살릴 수 있으며, `0.0 dB`로 두면 완전 무왜곡 VCA로 동작합니다.
