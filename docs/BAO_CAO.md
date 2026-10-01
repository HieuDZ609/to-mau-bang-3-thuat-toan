# BÁO CÁO: TÔ VÙNG ĐA GIÁC — BOUNDARY / SCANLINE / JORDAN FILL

**Nội dung:** 03 chương trình C++ độc lập, cùng một bộ giao diện, minh họa hoạt động của ba
thuật toán tô vùng đa giác với animation từng bước.
**Công nghệ:** C++17, OpenGL 3.3 (core profile), GLFW3, GLAD, GNU Make.
Hướng dẫn cài đặt và phím tắt: [`../README.md`](../README.md).

---

## 1. Bài toán tô vùng

Sau khi đường biên của đa giác đã được raster hóa thành tập pixel, ta cần xác định **những pixel
nào nằm bên trong** để tô chúng. Với người họa sĩ, đây là bước không thể thiếu: nếu không có nó,
đường nét đóng kín vẫn chỉ là một đường rỗng. Với người lập trình, đây là bài toán kinh điển
gọi là **polygon filling / scan conversion**, và mọi thuật toán tô vùng đều dựa trên cùng một
mệnh đề hình học duy nhất.

### 1.1. Nền tảng: định lý Jordan và quy tắc chẵn–lẻ

**Định lý Jordan.** Mọi đường cong đơn giản khép kín trong mặt phẳng (tức là đường cong không tự
cắt) chia mặt phẳng thành đúng hai miền: miền trong và miền ngoài. Với đa giác, đường cong đó
chính là chuỗi cạnh khép kín.

Từ đó suy ra cách kiểm tra một điểm `P` có nằm trong đa giác hay không:

> Kẻ một tia ngang từ `P` sang một phía bất kỳ (chọn phía không chạm đỉnh nào của đa giác để
> tránh trường hợp tia đi qua đỉnh). Đếm số lần tia cắt các cạnh của đa giác:
> **số lẻ ⇒ P nằm trong; số chẵn ⇒ P nằm ngoài.**

Đây gọi là **quy tắc chẵn–lẻ** (*even–odd rule* hay *crossing number*). Khi đi vòng quanh đa giác
một vòng, miền trong/ngoài phải đổi trạng thái mỗi lần cắt, nên sau một số lần chẵn ta trở về
trạng thái ban đầu — đó chính là lý do quy tắc chỉ cần **tính chẵn hay lẻ**.

Điểm mấu chốt của cả ba thuật toán trong bài này: chúng **không phải ba ý tưởng khác nhau**.
Chúng là ba cách cùng trả lời một câu hỏi: *"những pixel nào nằm trong?"*, khác nhau ở **đơn vị
xử lý** và **mức độ tận dụng kết quả tính được**.

| Thuật toán | Câu hỏi được trả lời | Đơn vị xử lý | Tái sử dụng kết quả |
|---|---|---|---|
| Jordan | pixel `(x,y)` này trong hay ngoài? | từng pixel | không, mỗi pixel tự quyết định |
| Scanline | cả dòng `y` gồm những đoạn nào? | từng dòng ngang | có, 1 lần tính cho cả dòng |
| Boundary | pixel nào liên thông với seed? | từng pixel theo lan truyền | có, dùng quan hệ lân cận |

## 2. Boundary Fill — tô theo đường biên

### 2.1. Ý tưởng

Không giải bài toán "pixel này trong hay ngoài" mà giải bài toán tương đương: **những pixel nào
liên thông với điểm seed mà không đi qua đường biên**. Vì đường biên đã raster hóa thành tập
pixel chặn, nên miền trong chính là thành phần liên thông chứa seed.

Cài đặt dùng **stack tường minh** (không đệ quy) để dễ quan sát trạng thái:

1. Raster hóa các cạnh đa giác bằng thuật toán Bresenham → đánh dấu các ô là *biên*.
2. Người dùng đặt **seed point**; seed bị từ chối nếu nằm trên biên hoặc đã được tô.
3. Lặp: lấy một ô đỉnh stack → đánh dấu *đang xử lý* → với mỗi lân cận chưa tô, đánh dấu
   *frontier* và đẩy vào stack.
4. Stack rỗng ⇒ đã tô xong toàn bộ thành phần liên thông.

Mã nguồn: `floodStep()` tại `boundary_fill.cpp:122`; bộ lân cận 4/8 hướng ở
`boundary_fill.cpp:126-129`; điều kiện từ chối seed ở `boundary_fill.cpp:105`.

### 2.2. Liên kết 4 và liên kết 8

Số lượng lân cận được xét quyết định hình dạng vùng tô:

```
4 liên kết            8 liên kết
  · — ·               · — · — ·
  |   |               |   |   |
  — · —               — — · — —
  |   |               |   |   |
  · — ·               · — · — ·
```

- **4 liên kết** chỉ đi theo 4 hướng: vùng tô bám sát đường biên, chỉ đi vòng được quanh đường
  biên dày 1 pixel.
- **8 liên kết** đi cả 4 hướng chéo: vùng tô lọt được qua các khe chéo mà 4 liên kết bị chặn,
  đổi lại dễ "rò" ra ngoài nếu đường biên có lỗ hổng điểm.

Đây là bản chất của vấn đề **8-connected / 4-connected** trong xử lý ảnh số. Chương trình cho
đổi bằng phím `B` để so sánh trực tiếp.

### 2.3. Đánh giá

| | |
|---|---|
| Ưu điểm | Cấu trúc rất đơn giản, dễ chứng minh tính đúng bằng quan hệ liên thông; animation trực quan vì thấy rõ biên lan và hàng đợi; không cần sắp xếp gì; dùng được cho vùng bất kỳ kể cả khi không có đường biên khép kín rõ ràng |
| Nhược 1 | Phải chọn seed point — cùng một hình, seed đặt sai chỗ thì ra kết quả khác (chỉ tô được một vùng con) |
| Nhược 2 | Chỉ tô được đúng một thành phần liên thông chứa seed; không tô hết được đa giác tự cắt (bát quá có 2 vùng) |
| Nhược 3 | Độ phức tạp O(N·W·H) với N = số pixel cần tô: phải duyệt cả vùng, tốn bộ nhớ stack |
| Nhược 4 | Phụ thuộc mạnh vào chất lượng raster hóa đường biên; đường biên rờ rạc dễ gây rò |

## 3. Scanline Fill — tô theo dòng quét

### 3.1. Ý tưởng

Nhận xét then chốt: **các điểm nằm trên cùng một đường thẳng ngang thì vùng bên trong là các đoạn
liên tiếp xen kẽ vào — ra**. Vì vậy thay vì hỏi từng pixel, ta hỏi cho **cả dòng** một lần: "dòng
`y` này cắt các cạnh ở những toạ độ x nào?" rồi ghép cặp `(x₀,x₁) (x₂,x₃) …` rồi tô.

Mỗi lần gặp một giao điểm, miền trong/ngoài lại đảo trạng thái, nên danh sách giao điểm phải có
số phần tử **chẵn** và được tô theo từng cặp — đây chính là quy tắc chẵn–lẻ của định lý Jordan
được áp dụng **theo cả dòng một lượt** thay vì theo từng pixel.

### 3.2. Active Edge Table (AET)

Để không phải duyệt toàn bộ N cạnh ở mỗi dòng, ta dùng bảng cạnh đang hoạt động. Mỗi cạnh
(`struct Edge` tại `scanline_fill.cpp:17`) lưu:

| Thành phần | Ý nghĩa |
|---|---|
| `yMin`, `yMax` | cạnh chỉ tồn tại trên các dòng `yMin ≤ y < yMax` |
| `x` | toạ độ x của giao điểm tại dòng hiện tại |
| `dxdy` | độ dốc nghịch đảo `Δx/Δy` — giao điểm trôi mỗi dòng một lượng `dxdy` |

Vì các dòng quét cách nhau đúng 1 đơn vị nên công thức cập nhật là hằng số, không cần chia lại:

```
x(y + 1) = x(y) + Δx/Δy
```

Quy trình xử lý **một dòng quét** (`scanStep()` tại `scanline_fill.cpp:130`):

1. **Loại cạnh hết hạn**: giữ lại các cạnh có `yMax > y` (`scanline_fill.cpp:140-143`).
2. **Thêm cạnh mới**: cạnh có `yMin == y` được đưa vào AET, với `x` hiệu chỉnh về đúng dòng `y`
   bằng `x += dxdy·(y − yMin)` (`scanline_fill.cpp:145-151`).
3. **Sắp xếp** AET tăng dần theo `x` — cần vì cạnh vào/ra không theo thứ tự vẽ
   (`scanline_fill.cpp:153`).
4. **Ghép cặp và tô**: lấy từng cặp liên tiếp, làm tròn về ô gần nhất rồi tô đoạn `[x₀, x₁]`
   (`scanline_fill.cpp:160-171`).
5. **Trôi giao điểm**: `x += dxdy` cho mọi cạnh trong AET (`scanline_fill.cpp:177`).

Cạnh nằm ngang (`y₁ == y₂`) bị loại khỏi bảng ngay từ đầu vì không tạo giao điểm với đường quét —
đây là tiêu chí đã dùng trong bộ test (mục 6).

### 3.3. Ví dụ thật từ log chương trình

Đa giác ngôi sao 10 đỉnh, mỗi lần `Space` quét 2 dòng:

```
[y= 2] AET=2 | giao: 22.0 22.0 | cap doi=1 | da to 1 px
[y= 3] AET=2 | giao: 21.6 22.4 | cap doi=1 | da to 2 px
[y= 4] AET=2 | giao: 21.2 22.8 | cap doi=1 | da to 5 px
[y=12] AET=2 | giao:  7.0 37.0 | cap doi=1 | da to 77 px
[y=23] AET=4 | giao: 14.4 22.0 22.0 29.6 | cap doi=2 | da to 282 px
[y=24] AET=4 | giao: 14.0 20.3 23.7 30.0 | cap doi=2 | da to 296 px
```

- `y=2`: đỉnh trên cùng nằm đúng trên dòng quét, hai cạnh kẹp tạo **hai giao điểm trùng nhau**
  (`22.0`, `22.0`) nên chỉ tô được 1 ô — đúng như mong đợi.
- `y=12`: hai giao điểm `7.0` và `37.0` là hai đỉnh ngoài cùng, đoạn tô dài 31 ô.
- `y=23`: AET có **4 cạnh**, 4 giao điểm ghép thành **2 đoạn** tô — đây là chỗ ngôi sao bị khuyết,
  một dòng quét phải cho ra nhiều đoạn rời nhau.
- `x` trôi đều mỗi dòng đúng `0.4` (`22.0 → 21.6 → 21.2 → …`) đúng như `dxdy` đã tính.

### 3.4. Đánh giá

| | |
|---|---|
| Ưu điểm | Nhanh: **O(N·H)** thay vì O(N·W·H) — không phải duyệt pixel để quyết định trong/ngoài; tận dụng tính chất đều của dòng quét nên `x` chỉ cộng thêm hằng số; dễ mở rộng để tô nhiều đường quét song song; animation theo dòng rất dễ quan sát |
| Nhược 1 | Phải sắp xếp giao điểm ở mỗi dòng |
| Nhược 2 | Đỉnh nằm đúng trên dòng quét cần quy ước cẩn thận, nếu không sẽ sinh đoạn thừa |
| Nhược 3 | Cần cấu trúc AET, bộ nhớ phụ thuộc số cạnh |

## 4. Jordan Fill — tô theo định lý Jordan

### 4.1. Ý tưởng

Đây là hình thức trực tiếp nhất của mệnh đề ở mục 1.1: **quét toàn bộ lưới pixel**, mỗi pixel là
một câu hỏi độc lập "pixel này có nằm trong đa giác không?". Không có biến trạng thái nào được
chia sẻ giữa các pixel, không stack, không bảng cạnh.

### 4.2. Thuật toán đếm giao điểm

Với pixel `P(px, py)`, duyệt toàn bộ cạnh `(p, q)` của đa giác:

1. Bỏ qua cạnh nằm ngang (`p.y == q.y`) — nó không cắt đường ngang `y = py`.
2. Cạnh chỉ có thể cắt nếu `min(p.y,q.y) ≤ py < max(p.y,q.y)`. Dùng `<` ở vế phải để **mỗi đỉnh
   chỉ được tính đúng một lần** khi tia đi qua đỉnh.
3. Giao điểm: `x = p.x + (py − p.y)·(q.x − p.x)/(q.y − p.y)`.
4. Nếu `x` nằm về phía đang quét của `px` thì `cnt++`.

Cuối cùng: `cnt` lẻ ⇒ tô, `cnt` chẵn ⇒ bỏ qua. Mã nguồn: `countCrossings()` tại
`jordan_fill.cpp:107`, quy tắc chẵn–lẻ tại `jordan_fill.cpp:143`.

### 4.3. Hướng tia không ảnh hưởng kết quả

Nếu tia quét sang phải hay sang trái thì **kết quả phải y hệt nhau**, chỉ khác ở chỗ *số* giao
điểm tìm được. Chương trình cho đổi hướng tia bằng phím `B` để kiểm chứng ngay trên màn hình.
Minh hoạ bằng log thật, cùng một đa giác:

```
(10, 7) so cat=2 -> NGOAI     ← tia sang phải: cắt 2 cạnh, kết luận NGOAI
(10, 7) so cat=0 -> NGOAI     ← tia sang trái: không cắt cạnh nào, kết luận NGOAI
(22, 3) so cat=1 -> TRONG     ← số lẻ ⇒ tô
```

Số lần cắt khác nhau (2 và 0) nhưng kết luận phải giống nhau — đây là tính chất được kiểm chứng tự
động trên cả 5 hình thử (mục 6).

### 4.4. Đánh giá

| | |
|---|---|
| Ưu điểm | Đúng bản chất với định lý Jordan, dễ chứng minh; **tự nhiên xử lý đa giác tự cắt** vì quy tắc chẵn–lẻ định nghĩa chính xác miền "bên trong"; không cần seed, không cần thứ tự cạnh; có thể dùng để kiểm tra điểm bất kỳ (ứng dụng vào hit-test) |
| Nhược 1 | Chậm: O(N·W·H), mỗi pixel duyệt lại toàn bộ cạnh |
| Nhược 2 | Log rất dài (1 dòng cho mỗi pixel) — chương trình chỉ in đầy đủ khi bước tay, chạy tự động thì in tiến trình |
| Nhược 3 | Không tận dụng được quan hệ giữa các pixel lân cận, không có khái niệm "dòng quét" |

## 5. So sánh ba thuật toán

| Tiêu chí | Boundary Fill | Scanline Fill | Jordan Fill |
|---|---|---|---|
| Bản chất | lan truyền theo lân cận | giải từng dòng ngang | kiểm tra từng điểm |
| Đơn vị xử lý | 1 pixel / bước | 1 dòng / bước | 1 pixel / bước |
| Cấu trúc dữ liệu | stack tường minh | AET + danh sách giao điểm | không cần |
| Trạng thái giữa các bước | có (stack) | có (AET, `x`, `y`) | không |
| Độ phức tạp | O(N·W·H) | **O(N·H)** | O(N·W·H) |
| Cần seed point | **có** | không | không |
| Đa giác tự cắt | chỉ tô được 1 vùng | tô đủ (quy tắc chẵn–lẻ) | tô đủ (quy tắc chẵn–lẻ) |
| Vùng không khép kín | tô được (nếu có biên chặn) | không | không |
| Dễ chứng minh | dễ (liên thông) | khó hơn (cần AET) | dễ (đếm giao điểm) |
| Dễ demo từng bước | rất trực quan | trực quan | trực quan |
| Dùng khi nào | cần tô vùng bất kỳ, ưu tiên minh bạch; tô vùng chọn tay | cần tốc độ, tô đa giác lớn | cần tính đúng tuyệt đối cho hình tự cắt; hit-test |

**Quan hệ giữa chúng:** Jordan và Scanline dùng **cùng một nguyên lý** (đếm giao điểm với đường
quét rồi áp dụng quy tắc chẵn–lẻ). Khác biệt là Jordan áp nguyên lý đó **độc lập cho từng pixel**,
còn Scanline áp nó **một lần cho cả dòng** rồi tô nguyên đoạn. Nói cách khác:

> Jordan = quy tắc chẵn–lẻ ở dạng nguyên thuỷ.
> Scanline = Jordan + tối ưu hoá bằng AET.
> Boundary = cách tiếp cận hoàn toàn khác: dùng tính liên thông thay cho đếm giao điểm.

Vì vậy với đa giác đơn giản, cả hai thuật toán phải cho ra cùng một tập pixel — điều này đã được
kiểm chứng bằng số liệu ở mục 6.

## 6. Kiểm chứng kết quả

### 6.1. Phương pháp

Vì ba chương trình viết bằng OpenGL nên không thể kiểm thử bằng framework unit test thông thường.
Cách kiểm chứng đã dùng: tách riêng **phần thuật toán** (không phụ thuộc đồ hoạ) khỏi phần vẽ,
biên dịch riêng phần đó rồi chạy trên một bộ hình thử cố định, và so kết quả với **một tham chiếu
chẵn–lẻ viết độc lập** trong chính bộ test. Bộ test gồm 5 hình: chữ nhật 10×5, tam giác, ngôi sao
10 đỉnh, bát quá tự cắt, ngũ nghi (ngôi sao 5 đỉnh vẽ bằng nét liền nên tự cắt).

Kết quả: **cả ba thuật toán đều đạt toàn bộ tiêu chí.**

### 6.2. Boundary Fill (chế độ 4-connected, seed bên trong)

| Hình | Số px tô | Tham chiếu | Số vùng bị cạnh tự cắt ngăn | Rò ra ngoài | Đúng 1 thành phần |
|---|---|---|---|---|---|
| Chữ nhật 10×5 | 36 | 36 | 0 | 0 | ✓ |
| Tam giác | 58 | 58 | 0 | 0 | ✓ |
| Ngôi sao 10 đỉnh | 239 | 239 | 0 | 0 | ✓ |
| Bát quá tự cắt | **56** | 112 | 1 | 0 | ✓ (chỉ 1 trong 2 vùng) |
| Ngũ nghi 5 đỉnh | **1** | 85 | 0 | 0 | ✓ |

(Rò ra ngoài = số pixel bị tô nhầm ra ngoài đa giác; cột "đúng 1 thành phần" kiểm tra tập pixel tô
có đúng bằng thành phần liên thông 4-connected chứa seed hay không.)

Các tiêu chí cấu trúc khác đều đạt:

- Số pixel biên của chữ nhật 10×5 = 30; trước khi đặt seed thì chưa tô pixel nào; stack sau khi
  đặt seed = 1; khi tô xong thì frontier và ô đang xử lý đều bằng 0.
- Chế độ 8-connected cho cùng kết quả 36 px với chữ nhật.
- Seed đặt **ngoài** đa giác → tô đúng 1342 px là vùng ngoài (1408 − 36 − 30) — chứng minh thuật
  toán thực sự tô *thành phần liên thông chứa seed*, không phải tô theo vùng cố định.
- Seed đặt **trên đường biên** bị từ chối (0 lần chấp nhận).
- Vuông 4×4 → 9 px bên trong; ngôi sao → 239 px.

**Nhận xét quan trọng:** hai dòng bị lệch lớn (bát quá, ngũ nghi) **không phải lỗi thuật toán** mà
là hệ quả tất yếu của định nghĩa: flood fill chỉ tô được thành phần liên thông chứa seed. Với bát
quá có hai vùng riêng biệt nên tô được 56/112 px; với ngũ nghi các vùng con bị các đoạn cạnh tự
cắt ngăn ra và vùng chứa seed chỉ còn 1 px. Muốn tô hết phải đặt lại seed cho từng vùng — đây
chính là hạn chế đã nêu ở mục 2.3.

### 6.3. Scanline Fill

| Hình | Số px tô | Lệch ≤ 0.5 px | Lệch > 0.5 px |
|---|---|---|---|
| Chữ nhật 10×5 | 55 | 5 | **0** |
| Tam giác | 92 | 10 | **0** |
| Ngôi sao 10 đỉnh | 330 | 32 | **0** |
| Bát quá tự cắt | 167 | 17 | **0** |
| Ngũ nghi 5 đỉnh | 128 | 18 | **0** |

Không có pixel nào lệch quá 0.5 px so với tham chiếu chẵn–lẻ; sai số chỉ nằm ở các ô nằm ngay
trên đường biên, đúng như bản chất thuật toán có làm tròn về ô gần nhất.

Các tiêu chí cấu trúc AET đều đạt: với tam giác `(5,5)(20,5)(12,15)` phải có `yStart=5`,
`yEnd=15`, và **chỉ 2 cạnh** được đưa vào AET (cạnh ngang bị loại đúng như thiết kế); dòng quét
đầu tiên có 2 giao điểm và tô 16 px; quét xong đạt 92 px; sau khi `R` (reset) về 0 px và con trỏ
dòng quét trở lại đầu.

### 6.4. Jordan Fill

| Hình | px trong (tia phải) | px trong (tia trái) | Sai số ngoài biên |
|---|---|---|---|
| Chữ nhật 10×5 | 50 | 50 | 0 |
| Tam giác | 82 | 83 | 0 |
| Ngôi sao 10 đỉnh | 298 | 298 | 0 |
| Bát quá tự cắt | **150** | **150** | 0 |
| Ngũ nghi 5 đỉnh | 110 | 114 | 0 |

- Kết quả khớp tham chiếu chẵn–lẻ ở **0 pixel sai** (ngoài biên) trên cả 5 hình.
- Tia phải và tia trái cho **cùng một tập pixel ngoài biên** trên cả 5 hình — chênh lệch chỉ nằm ở
  các ô nằm ngay trên đường biên (82/83 với tam giác, 110/114 với ngũ nghi), do làm tròn toạ độ
  giao điểm chứ không phải do kết luận trong/ngoài khác nhau.
- Bát quá tự cắt cho **150 px = đúng 2 tam giác**, khớp tham chiếu — đây là ưu thế rõ ràng của
  Jordan so với Boundary Fill (56 px).

### 6.5. Quy ước tính pixel: giải thích khác biệt 36 và 50

Ở chữ nhật 10×5, Boundary Fill tô 36 px còn Jordan đếm 50 px. Đây **không phải sai lệch kết quả**
mà là khác biệt về quy ước, đã đối chiếu trực tiếp trên cùng hình:

| Tập pixel | Số lượng | Vùng bao phủ |
|---|---|---|
| Đường biên raster hoá (Bresenham) | 30 | x ∈ [5,15], y ∈ [5,10] — nằm **đúng trên cạnh** đa giác |
| Boundary Fill tô | 36 | x ∈ [6,14], y ∈ [6,9] — phần **trong nghiêm**, không ghi đè biên |
| Jordan đếm là trong | 50 | x ∈ [5,14], y ∈ [5,9] — gồm cả các ô có tâm nằm **trên cạnh** |

Kiểm tra tập hợp: **36 pixel do Boundary Fill tô đều nằm trong tập 50 pixel của Jordan** (chênh
lệch 0 theo hướng ngược lại), và 14 pixel chênh lệch chính là các ô nằm ngay trên cạnh đa giác
(hàng `y=5` và cột `x=5`) — do quy tắc khoảng bán mở `min ≤ py < max` của Jordan tính chúng là
"trong". Nói cách khác: **Boundary Fill tô phần trong và giữ nguyên đường biên**, còn
Jordan/Scanline đếm cả các ô có tâm nằm trên biên. Khi so sánh hai thuật toán phải dùng cùng
một quy ước này.

## 7. Thiết kế giao diện

### 7.1. Nguyên tắc chung

Cả ba chương trình dùng chung một bộ khung hiển thị nhưng **không dùng chung mã nguồn** — mỗi
file là một bài hoàn chỉnh, đúng yêu cầu "3 bài riêng biệt". Các quyết định thiết kế chung:

| Quyết định | Giá trị | Lý do |
|---|---|---|
| Kích thước lưới | 44 × 32 ô, mỗi ô 28 px | ô lớn để quan sát rõ từng pixel khi demo |
| Kích thước cửa sổ | 1232 × 896 px | đúng bằng lưới, không dùng thanh cuộn |
| Nền | trắng, `glClearColor(1,1,1,1)` | dễ phân biệt vùng đã tô với nền |
| Đường lưới | xám nhạt `#E5EBF2`, mỗi 5 ô vẽ đậm `#CCD1DB` | giúp đếm tọa độ khi click đỉnh |
| Chữ trong cửa sổ | **không dùng** | hướng dẫn và log in ra terminal bằng `printf` |
| Nhập đa giác | click chuột trái từng đỉnh, `Enter` đóng | không cần hình mẫu cứng |
| Animation | 1 bước = 1 px (boundary/jordan) hoặc 1 dòng (scanline), `T` chạy tự động, `[` `]` đổi tốc độ | thấy rõ từng bước của thuật toán |

Nguyên tắc quan trọng nhất về màu: **chỉ tô ô có trạng thái màu, ô trống để nguyên nền trắng**.
Nhờ đó phần nền và đường lưới của cả ba chương trình hiển thị giống hệt nhau, người xem so sánh
được thuật toán này với thuật toán kia mà không bị phân tâm bởi nền.

### 7.2. Quy ước màu riêng của từng chương trình

| Ý nghĩa | Boundary Fill | Scanline Fill | Jordan Fill |
|---|---|---|---|
| Đường biên | cam `#E45730` | xám đen `#1D1D1D` | xám đen `#212129` |
| Vùng đã tô | xanh lam `#2A9D8F` | gradient vàng → cam theo dòng | xanh dương `#3A86FF` |
| Đang xử lý | cam (ô vừa lấy khỏi stack) | dải hồng dòng quét `#FFBFC8` | tia ngang cam nhạt `#FFB4A2`, ô đang xét vàng `#FFD166` |
| Đang chờ | vàng (frontier trong stack) | giao điểm chính xác: ô xanh dương | giao điểm đầu tiên của tia: ô đỏ |
| Đỉnh đa giác | ô xanh dương đậm | ô xanh dương đậm | ô xanh dương đậm |

Mỗi màu đều gắn với một khái niệm của thuật toán tương ứng, nên khi nhìn animation có thể đọc
được diễn biến thuật toán mà không cần đọc log.

### 7.3. Log minh hoạ

Log ra terminal là phần "bản vẽ" thứ hai của mỗi thuật toán:

- **Boundary Fill:** số pixel đã tô và kích thước stack sau mỗi bước — thấy rõ hàng đợi xếp
  và rỗng dần.
- **Scanline Fill:** in AET, toạ độ các giao điểm, số cặp đoạn và tổng px — đây là bản chất toán
  của thuật toán nên giá trị lý thuyết cao nhất.
- **Jordan Fill:** in toạ độ từng pixel cùng số lần cắt và kết luận TRONG/NGOAI.

## 8. Kết luận

1. Cả ba thuật toán đều từ cùng một nền tảng hình học: **quy tắc chẵn–lẻ của định lý Jordan** (trừ
   Boundary Fill dùng cách tiếp cận liên thông). Vì vậy với đa giác đơn giản, kết quả tô phải trùng
   nhau — và điều này đã được kiểm chứng bằng số liệu, không chỉ bằng lập luận.
2. Khác biệt thực chất nằm ở **đơn vị xử lý**: Jordan hỏi từng pixel, Scanline hỏi từng dòng,
   Boundary hỏi từng lân cận. Đây quyết định cả tốc độ lẫn hành vi với hình tự cắt.
3. **Jordan chính xác nhất** về mặt định nghĩa (tô đủ 150/150 px cho bát quá), **Scanline nhanh
   nhất** (O(N·H) thay vì O(N·W·H)), **Boundary trực quan và đơn giản nhất** nhưng phụ thuộc seed
   và chỉ tô được một vùng.
4. Phần animation không chỉ cho minh hoạ mà còn **chứng minh trực quan** các khái niệm trừu
   tượng: biên lan và stack (Boundary), bảng AET (Scanline), tia và số lần cắt (Jordan).

## 9. Hướng phát triển

- **Khử răng cưa (anti-aliasing):** hiện tô bằng cách tô nguyên ô, biên bị lấp cưa; có thể tô theo
  độ phủ một phần (coverage) cho mượt hơn.
- **Đa giác có lỗ hổng / nhiều đường biên:** Boundary Fill có lợi thế ở đây vì tô theo thành phần
  liên thông, còn Jordan/Scanline cần xử lý thêm các cạnh ngoài.
- **Đa giác hàng nghìn đỉnh:** có thể dùng cấu trúc không gian (uniform grid, quadtree) để giảm
  số cạnh phải duyệt ở mỗi dòng của Scanline.
- **Song song hoá:** ghép cặp giao điểm và tô từng dòng của Scanline là ứng viên tốt để chạy
  trên GPU bằng shader.
- **Quy tắc khác ngoài chẵn–lẻ:** quy tắc winding number cho ra vùng tô khác với chẵn–lẻ khi
  đa giác tự cắt, đáng để so sánh thêm.

## 10. Kịch bản trình bày (khoảng 10 phút)

| Thời gian | Nội dung | Thao tác |
|---|---|---|
| 0:00–1:00 | Đặt vấn đề: đường biên đã kín thì làm sao biết pixel nào trong? Nêu định lý Jordan | — |
| 1:00–1:30 | Giới thiệu 3 chương trình, chỉ ra chúng khác nhau ở đơn vị xử lý | — |
| 1:30–3:30 | Chạy `boundary_fill`: click đa giác, `Enter`, đặt seed, `Space` từng bước, nhấn `B` đổi 4/8 liên kết | giải thích biên lan, frontier, stack |
| 3:30–6:00 | Chạy `scanline_fill`: `Space` từng dòng, chỉ ra dải hồng và log AET, dừng ở dòng có AET=4 để giải thích ghép cặp | giải thích `x += dxdy`, ưu điểm O(N·H) |
| 6:00–8:00 | Chạy `jordan_fill`: `Space` từng pixel, nhấn `B` đổi hướng tia, chỉ ra số lần cắt chẵn/lẻ | giải thích tại sao không cần seed, tự cắt được |
| 8:00–9:00 | Vẽ bát quá (4 đỉnh) chạy cả 3 chương trình: boundary chỉ tô 1 vùng, hai thuật toán kia tô đủ | kết luận số liệu mục 6 |
| 9:00–10:00 | Tổng kết bảng so sánh, nêu hạn chế và hướng phát triển | — |

Chuẩn bị trước: hai hình dùng nhiều nhất là **ngôi sao 10 đỉnh** (có chỗ khuyết để thấy dòng quét
sinh 2 đoạn) và **bát quá 4 đỉnh** (để minh hoạ hạn chế của Boundary Fill).

## 11. Phụ lục

### 11.1. Bảng điều hướng mã nguồn

| Nội dung | Boundary Fill | Scanline Fill | Jordan Fill |
|---|---|---|---|
| Raster hoá đường biên | `bresLine()` `:46`, `rasterizePolygon()` `:83` | `bresLine()` `:56`, raster trong `closePolygon()` `:95` | `bresLine()` `:49`, raster trong `closePolygon()` `:87` |
| Hàm thuật toán chính | `floodStep()` `:122` | `scanStep()` `:130` | `countCrossings()` `:107`, `jordanStep()` `:129` |
| Quy tắc chẵn–lẻ / ghép cặp | — | vòng ghép cặp `:160-171` | điều kiện `c % 2 == 1` `:143` |
| Dựng hình học để vẽ | `buildGeometry()` `:188` | `buildGeometry()` `:216` | `buildGeometry()` `:192` |
| Xử lý phím | `key_callback()` `:277` | `key_callback()` `:313` | `key_callback()` `:282` |

(Số dòng theo phiên bản hiện tại của mã nguồn.)

### 11.2. Các con số đã kiểm chứng

- Lưới: 44 × 32 = **1408 ô**.
- Chữ nhật 10×5: biên 30 px; boundary tô 36 px; jordan đếm 50 px; 36 px tô đều thuộc tập 50 px.
- Ngôi sao 10 đỉnh: boundary 239 px; jordan 298 px; scanline 330 px (0 px lệch quá 0.5 px).
- Bát quá tự cắt: boundary **56/112** px; jordan **150/150** px; scanline 167 px, 0 px sai.
- Seed ngoài đa giác (chữ nhật): boundary tô **1342** px = 1408 − 36 − 30.
- Vuông 4×4: 9 px bên trong.

### 11.3. Cách chạy lại bộ kiểm chứng

Bộ test không nằm trong thư mục bài này (nó phụ thuộc việc tách riêng phần thuật toán khỏi phần
đồ hoạ). Cách kiểm chứng lại độc lập, không cần công cụ gì thêm:

1. Chạy `boundary_fill`, vẽ một hình đơn giản, đặt seed, nhấn `G`; đếm pixel tô trong terminal
   rồi đối chiếu với số ô màu xanh lam quan sát được trên lưới.
2. Chạy `jordan_fill` với **cùng hình đó**, nhấn `G`; kết quả phải bao trọn vùng boundary tô
   được (vùng của Jordan rộng hơn hoặc bằng, do tính thêm các ô nằm trên biên).
3. Chạy `scanline_fill` với cùng hình; vùng tô phải trùng vùng của Jordan.
4. Vẽ bát quá 4 đỉnh `(5,5)(25,5)(5,20)(25,20)` và lặp lại các bước trên: boundary chỉ tô được
   một nửa, hai chương trình kia tô đủ hai tam giác.
