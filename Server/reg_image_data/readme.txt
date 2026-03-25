이미지파일 이름을 UUID 변환 후 아래 테이블에 더미데이터 삽입

menus;
menu_categories;
restaurants;
food_categories;

서버에서 아래 실행
---------------------------------
chmod 700 migrate.sh
./migrate.sh
mysql -u bemin -p bemin_db < insert_bemin_data.sql
---------------------------------

폴더
---------------------------------
(기존)
음식사진 : 원본 이미지 파일 폴더

(자동생성)
images :  이미지파일 복사본(파일명은 UUID로 변환)
---------------------------------

파일
---------------------------------
(기존)
migrate.sh

(자동생성)
insert_bemin_data.sql
---------------------------------

===============================================
아래 테이블에 데이터를 삭제

menus;
menu_categories;
restaurants;
food_categories;

서버에서 아래 실행
---------------------------------
chmod 700 reset_tables.sql
mysql -u bemin -p bemin_db < reset_tables.sql
---------------------------------

파일
---------------------------------
(기존)
reset_tables.sql
---------------------------------

