-- [주의] 실행 시 데이터가 즉시 삭제되며 복구할 수 없습니다.
-- 대상 테이블: menus, menu_categories, restaurants, food_categories

-- 1. 외래 키 제약 조건 잠시 해제 (참조 오류 방지)
SET FOREIGN_KEY_CHECKS = 0;

-- 2. 테이블 비우기 및 ID 초기화 (1번부터 다시 시작되도록)
TRUNCATE TABLE menus;
TRUNCATE TABLE menu_categories;
TRUNCATE TABLE restaurants;
TRUNCATE TABLE food_categories;

-- 3. 외래 키 제약 조건 다시 활성화
SET FOREIGN_KEY_CHECKS = 1;

-- 4. 완료 메시지 출력
SELECT 'All 4 tables have been truncated.' AS result;
