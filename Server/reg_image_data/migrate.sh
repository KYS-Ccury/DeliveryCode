#!/bin/bash

# 설정을 위한 초기화
OUTPUT_DIR="./images"
SQL_FILE="insert_bemin_data.sql"
mkdir -p "$OUTPUT_DIR"
echo "SET FOREIGN_KEY_CHECKS = 0;" > $SQL_FILE

# ID 관리를 위한 변수 (간단한 매칭을 위해)
CAT_ID=0
REST_ID=0
MCAT_ID=0

# 1. 대분류 (치킨, 패스트푸드, 피자)
for cat_path in ./음식사진/*; do
    if [ -d "$cat_path" ]; then
        CAT_NAME=$(basename "$cat_path")
        CAT_ID=$((CAT_ID + 1))
        echo "INSERT INTO food_categories (category_id, category_name, sort_order) VALUES ($CAT_ID, '$CAT_NAME', $CAT_ID);" >> $SQL_FILE
        
        # 2. 식당/브랜드 (bbq, bhc, 맥도날드 등)
        for rest_path in "$cat_path"/*; do
            if [ -d "$rest_path" ]; then
                REST_NAME=$(basename "$rest_path")
                REST_ID=$((REST_ID + 1))
                # owner_id는 임의로 1번으로 통일, 주소 등은 테스트용 데이터
                echo "INSERT INTO restaurants (restaurant_id, owner_id, category_id, restaurant_name, biz_no, address, min_order_amt) VALUES ($REST_ID, 1, $CAT_ID, '$REST_NAME', '123-45-67890', '서울시 강남구 테스트로 $REST_ID', 15000);" >> $SQL_FILE
                
                # 3. 메뉴 카테고리 (후라이드, 세트메뉴 등)
                for mcat_path in "$rest_path"/*; do
                    if [ -d "$mcat_path" ]; then
                        MCAT_NAME=$(basename "$mcat_path")
                        MCAT_ID=$((MCAT_ID + 1))
                        echo "INSERT INTO menu_categories (menu_category_id, restaurant_id, category_name) VALUES ($MCAT_ID, $REST_ID, '$MCAT_NAME');" >> $SQL_FILE
                        
                        # 4. 메뉴 (이미지 파일)
                        find "$mcat_path" -type f \( -name "*.png" -o -name "*.jpg" \) | while read -r img_file; do
                            FULL_FILENAME=$(basename "$img_file")
                            # 파일명에서 '이름_가격.png' 분리
                            CLEAN_NAME="${FULL_FILENAME%.*}"
                            MENU_NAME="${CLEAN_NAME%_*}"
                            PRICE="${CLEAN_NAME##*_}"
                            
                            # 가격이 숫자가 아니면 0으로 기본값 (로고 파일 등 제외)
                            if ! [[ "$PRICE" =~ ^[0-9]+$ ]]; then PRICE=0; fi
                            
                            # UUID 생성 및 파일 복사
                            EXT="${img_file##*.}"
                            UUID_NAME=$(uuidgen)."$EXT"
                            cp "$img_file" "$OUTPUT_DIR/$UUID_NAME"
                            
                            # 메뉴 INSERT
                            echo "INSERT INTO menus (menu_category_id, menu_name, price, image_url, description) VALUES ($MCAT_ID, '$MENU_NAME', $PRICE, '/images/menus/$UUID_NAME', '$MENU_NAME 맛집입니다.');" >> $SQL_FILE
                        done
                    fi
                done
            fi
        done
    fi
done

echo "SET FOREIGN_KEY_CHECKS = 1;" >> $SQL_FILE
echo "작업 완료: $SQL_FILE 생성 및 $OUTPUT_DIR 에 이미지 저장됨."
