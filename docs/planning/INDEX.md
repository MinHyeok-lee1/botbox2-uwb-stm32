# UWB Advancement Roadmap - Customer Planning Index

Status: design only. Runtime code is unchanged.

## Customer Responsibility

`03_stm32-dw3000-G474RE-customer` is the sanitized delivery repository. It should receive only accepted Product changes from 02, with metadata sanitation and delivery artifact verification. It should not independently redesign ranging, CAN, low-power, scheduler, or high-rate behavior.

## Current Relevant Code Anchors

- Customer currently carries Product-derived ranging and low-power code in `Core/Src/main.c`.
- Customer low-power default is documented in-source around the Product/Customer delivery note at `Core/Src/main.c:5004`.
- Customer boundary scanner sanitation remains the required guard before any delivery update.

## Linked Product Plans

- Product detailed plans are owned by the 02 Product planning package. Customer docs intentionally avoid sibling repository paths.
- If 02 later accepts precision, STOP1, multi-anchor, 128 Hz, or responsiveness changes, 03 work should be a separate 02 -> 03 propagation task:
  - compare Product hunk;
  - adapt only accepted runtime behavior;
  - sanitize comments/docs;
  - re-export delivery firmware/manifest;
  - verify SHA alignment and scanner `FAIL=0 WARN=0`.

## Do Not Start Without Approval

- Do not implement 128 Hz or scheduler changes directly in Customer.
- Do not add internal Product analysis or experiment logs to Customer docs.
