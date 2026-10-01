# TÔ VÙNG THEO 3 THUẬT TOÁN — Boundary / Scanline / Jordan Fill

Ba chương trình C++ độc lập, mỗi chương trình minh họa **một thuật toán tô vùng đa giác** (polygon
filling) bằng cùng một bộ khung giao diện: cùng lưới, cùng bảng màu nền, cùng cách nhập đa giác,
cùng kiểu animation từng bước.

| Chương trình | Thuật toán | Đơn vị xử lý | Đặc điểm |
|---|---|---|---|
| `boundary_fill` | Tô theo đường biên (4/8-connected) | từng pixel lan truyền theo stack | Cần seed point, nhìn rõ cơ chế frontier |
| `scanline_fill` | Tô theo dòng quét (Active Edge Table) | từng dòng ngang | Log ra AET và các cặp đoạn tô |
| `jordan_fill` | Tô theo định lý Jordan (quy tắc chẵn–lẻ) | từng pixel, quyết định độc lập | Log số lần cắt của tia với từng cạnh |

Không có logic dùng chung giữa 3 chương trình — mỗi file là một bài hoàn chỉnh, đọc hiểu được
độc lập. Phần báo cáo lý thuyết + so sánh + kết quả kiểm chứng nằm ở [`docs/BAO_CAO.md`](docs/BAO_CAO.md).

---

## 1. Yêu cầu môi trường

| Thành phần | Yêu cầu |
|---|---|
| Hệ điều hành | Linux |
| Trình biên dịch | `g++` hỗ trợ C++17 (đã kiểm tra với GCC 15.2) |
| Đồ hoạ | OpenGL 3.3 core profile, driver có hỗ trợ |
| Thư viện | GLFW3 (`libglfw3`), GLAD (dùng để nạp hàm OpenGL), pthread, X11, m, dl |
| Header | `Dependencies/include` — chứa `glad/`, `GLFW/`, `KHR/`, `glm/`, `stb_image.h` |

`Dependencies/include` đã chứa sẵn các header cần dùng: `glad/`, `GLFW/`, `KHR/`
(khoảng 500 KB) — không cần tải thêm gì. GLAD được lấy qua header `<glad/glad.h>`, mã nguồn
GLAD nằm ở `src/glad.c`.

Nếu muốn dùng bộ header dùng chung của môn học thay cho bản trong repo:

```bash
cd FILL
rm -rf Dependencies
ln -s /duong/dan/den/Dependencies Dependencies
```

## 2. Biên dịch

```bash
cd FILL
make
```

Kết quả: 3 file thực thi `boundary_fill`, `scanline_fill`, `jordan_fill`.
Dọn sạch sản phẩm biên dịch: `make clean`.

Cờ biên dịch trong `Makefile`: `-std=c++17 -O2 -g -Wall`, liên kết
`-lglfw -lGL -lX11 -lpthread -ldl -lm`.

## 3. Chạy và sử dụng

```bash
./boundary_fill
./scanline_fill
./jordan_fill
```

Trong cửa sổ:

1. **Click chuột trái** lần lượt tại các đỉnh của đa giác (tọa độ hiển thị ở terminal).
2. **Enter** để đóng đa giác — chương trình tự raster hóa đường biên.
   Riêng `boundary_fill` yêu cầu thêm: **click chuột trái vào một điểm bên trong** để đặt seed point.
3. **Space** tiến 1 bước, **T** chạy tự động, **G** tô hết ngay (phím sẽ giải thích thêm ở terminal).

Cửa sổ không hiển thị chữ; toàn bộ hướng dẫn và log thuật toán in ra **terminal**.

### 3.1. Phím tắt

| Phím | Boundary Fill | Scanline Fill | Jordan Fill |
|---|---|---|---|
| Click trái | thêm đỉnh / đặt seed point | thêm đỉnh | thêm đỉnh |
| `Enter` | đóng đa giác (≥ 3 đỉnh) | đóng đa giác | đóng đa giác |
| `Space` | tô 1 bước | quét 1 dòng | xét 1 pixel |
| `T` | chạy tự động | chạy tự động | chạy tự động |
| `G` | tô hết ngay | quét hết ngay | quét hết ngay |
| `R` | xoá vùng đã tô, giữ đường biên | quét lại từ `y = yMin` | quét lại từ pixel (0,0) |
| `B` | đổi 4-connected ↔ 8-connected | — | đổi hướng tia (phải ↔ trái) |
| `[` / `]` | giảm / tăng tốc độ (1…1024 px/bước) | 1…64 dòng/bước | 1…512 px/bước |
| `X` | xoá vùng đã tô, giữ đường biên | — | — |
| `C` | xoá toàn bộ (kể cả đa giác) | xoá toàn bộ | xoá toàn bộ |
| `Backspace` | bỏ đỉnh cuối (khi chưa đóng đa giác) | bỏ đỉnh cuối | bỏ đỉnh cuối |
| `Esc` | thoát | thoát | thoát |

Mặc định mỗi lần nhấn phím tiến **1 px/dòng**; dùng `[` `]` để tăng tốc khi demo.

### 3.2. Quy ước màu

Lưới: 44 × 32 ô, mỗi ô 28 px, cửa sổ 1232 × 896 px. Nền trắng, đường lưới xám
(`#E5EBF2` lưới nhỏ, `#CCD1DB` lưới lớn 5 ô), đỉnh đa giác là ô xanh dương đậm.

| Ý nghĩa | Boundary Fill | Scanline Fill | Jordan Fill |
|---|---|---|---|
| Đường biên | cam `#E45730` | xám đen `#1D1D1D` | xám đen `#212129` |
| Vùng đã tô | xanh lam `#2A9D8F` | gradient vàng → cam theo từng dòng | xanh dương `#3A86FF` |
| Trạng thái đang xử lý | cam (pixel vừa lấy ra stack) | dải hồng dòng quét hiện tại `#FFBFC8` | tia ngang cam nhạt `#FFB4A2` + pixel đang xét vàng `#FFD166` |
| Đang chờ xử lý | vàng (frontier trong stack) | các giao điểm chính xác (ô xanh dương) | giao điểm đầu tiên của tia (ô đỏ) |

Nguyên tắc chung của cả 3 giao diện: **chỉ tô ô có trạng thái màu**, ô trống để nguyên nền
trắng — nên phần nền/đường lưới của 3 chương trình hiển thị giống hệt nhau.

## 4. Log mẫu (thật, chạy với đa giác ngôi sao 10 đỉnh)

### 4.1. Boundary Fill

```
=========================================================
   TO THEO DUONG BIEN (Boundary Fill)
   Luu do: vien do = do cam  |  da to = xanh lam
   frontier (trong stack) = vang  |  dang xu ly = cam
---------------------------------------------------------
[Click trai] them dinh da giac / dat seed point
[ENTER] dong da giac (it nhat 3 dinh)
[SPACE] to 1 buoc | [T] chay tu dong | [G] to het ngay
[B] doi 4/8-connected | [ ] tang/giam toc do
[X]/[R] xoa vung to | [C] xoa het | [BACKSPACE] bo dinh
[ESC] thoat
=========================================================
[Click] Dinh 1: (22, 2)
[Click] Dinh 2: (26, 12)
...
[ENTER] Da dong da giac 10 dinh, duong bien da duoc raster hoa.
       Bay gio click chuot trai vao mot diem de dat seed point.
[SEED] (22,12) | 4/8-connected: 4 | Toc do: 1 px/buc
[Xong] To xong 239 pixel trong che do 4-connected.
```

Ngoài ra mỗi lần nhấn `Space` chương trình in một dòng theo định dạng
`[Buoc] da to <so px> px | stack=<do dai stack>`.

Ý nghĩa: `frontier` = các ô đã đánh dấu là sẽ được tô nhưng còn nằm trong stack; mỗi lần
`Space` lấy tối đa `Tốc độ` ô ra khỏi stack, đánh dấu thành ô vừa xử lý, rồi đẩy các lân cận
chưa tô vào stack.

### 4.2. Scanline Fill

```
[ENTER] Da dong da giac 8 canh, y tu 2 den 29 (28 dong quet).
[SPACE]quet 1 dong | [T] chay tu dong | [G]quet het ngay
[y= 2] AET=2 | giao: 22.0 22.0 | cap doi=1 | da to 1 px
[y= 3] AET=2 | giao: 21.6 22.4 | cap doi=1 | da to 2 px
[y= 4] AET=2 | giao: 21.2 22.8 | cap doi=1 | da to 5 px
[y= 9] AET=2 | giao: 19.2 24.8 | cap doi=1 | da to 30 px
[y=12] AET=2 | giao:  7.0 37.0 | cap doi=1 | da to 77 px
[y=23] AET=4 | giao: 14.4 22.0 22.0 29.6 | cap doi=2 | da to 282 px
[y=24] AET=4 | giao: 14.0 20.3 23.7 30.0 | cap doi=2 | da to 296 px
```

Mỗi dòng: `y` = dòng đang quét, `AET` = số cạnh đang hoạt động, `giao` = toạ độ x của các giao
điểm (đã sắp xếp tăng dần), `cap doi` = số cặp đoạn được tô, `da to` = tổng pixel đã tô.
Dòng `y=23` minh hoạ trường hợp AET có 4 cạnh nên 4 giao điểm ghép thành 2 đoạn.

### 4.3. Jordan Fill

```
[ENTER] Da dong da giac 10 dinh. Bat dau quet tu pixel (0,0).
[B] doi huong tia: hien tai sang PHAI
( 0, 0) so cat=0 -> NGOAI
( 1, 0) so cat=0 -> NGOAI
...
(22, 3) so cat=1 -> TRONG
(21, 5) so cat=1 -> TRONG
(10, 7) so cat=2 -> NGOAI
```

`so cat` = số cạnh cắt tia ngang xuất phát từ pixel đang xét; số lẻ là **TRONG**, số chẵn là
**NGOAI**. Đổi hướng tia bằng `B` phải cho ra cùng kết quả (đã kiểm chứng tự động).

## 5. Cấu trúc thư mục

```
FILL/
├── README.md              <- file này
├── Makefile               <- build 3 chương trình
├── boundary_fill.cpp      <- Boundary Fill (stack tường minh, 4/8-connected)
├── scanline_fill.cpp      <- Scanline Fill (Active Edge Table)
├── jordan_fill.cpp        <- Jordan Fill (quy tắc chẵn–lẻ, point-in-polygon)
├── src/glad.c             <- mã nguồn GLAD (bản copy, không sửa)
├── Dependencies/include/  <- header: glad/, GLFW/, KHR/
└── docs/BAO_CAO.md        <- báo cáo: lý thuyết, so sánh, kết quả kiểm chứng
```

Mỗi chương trình là một file duy nhất, không chia nhỏ hàm theo file khác, không dùng chung mã
với nhau — đúng yêu cầu "3 bài riêng biệt".
