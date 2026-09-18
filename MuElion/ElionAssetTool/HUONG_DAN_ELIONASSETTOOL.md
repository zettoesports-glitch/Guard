# Hướng Dẫn Chi Tiết Sử Dụng ElionAssetTool.exe
Chuyển đổi và phục hồi tài nguyên MU Online gốc sang định dạng Elion Client x64 Vulkan

---

## 1. Giới thiệu & Cơ chế cốt lõi

`ElionAssetTool.exe` là công cụ dòng lệnh (CLI x64) độc lập được phát triển để phục vụ pipeline tài nguyên của Elion Client. Công cụ giải quyết triệt để các hạn chế về định dạng cũ của MU Online cổ điển, tối ưu hóa dữ liệu cho kiến trúc **Vulkan x64**, bảo mật và nén tài nguyên hiện đại.

### 1.1. Các nguyên tắc an toàn quan trọng
1. **Không ghi đè tự động (Fail-Safe Overwrite):**
   * Công cụ **từ chối ghi đè** lên bất kỳ file đầu ra nào nếu nó đã tồn tại (`refusing to overwrite...`).
   * **Quy trình chuẩn:** Luôn xuất dữ liệu ra một thư mục tạm thời (**staging directory**) sạch trước khi đồng bộ vào `Elion Client\data`.
2. **Bảo toàn file gốc:**
   * Cây tài nguyên MU Online gốc (thường nằm ở `Mu Online ORG\Data`) luôn được xử lý ở chế độ chỉ đọc (**read-only**).
3. **Mã hóa và kiểm tra toàn vẹn theo Domain:**
   * Mỗi nhóm tài nguyên thuộc về một Domain riêng biệt với khóa Blowfish 56-byte riêng (`data`, `model`, `texture`, `terrain`).
   * Mỗi container đều có mã kiểm tra tính toàn vẹn CRC32 và nén `zlib`.

### 1.2. Định dạng tài nguyên đầu ra của Elion
* **Cơ sở dữ liệu / Bảng thông số:** Đuôi `.datx` (FourCC: `DATX`, Domain: `data`).
* **Texture 2D / 3D:** Luôn gồm **cặp 2 file song hành**:
  * File thân dữ liệu ảnh (DDS RGBA8 UNORM mã hóa): `.jtex` (từ OZJ/JPG/BMP) hoặc `.ttex` (từ OZT/TGA).
  * File header mô tả (Sidecar header): `.jtexh` hoặc `.ttexh` (lưu width, height, sampler, format).
* **Mô hình 3D (3D Model):** Chuyển từ 1 file `.bmd` thành **bộ 4 file**:
  * `.emhd` (Model Header): Thông tin khung xương, cây phân cấp xương cha - con, action count.
  * `.emdl` (Model Data): Mesh vertex, triangle, normal, texture mapping, bounding box và sphere.
  * `.eani` (Animation Data): Quỹ đạo chuyển động (Quaternion rotation + vector translation).
  * `.egpu` (Vulkan GPU Buffer): Vertex/Index buffer dạng half-float 16-bit (`Float16`) nén tối ưu cho Vulkan pipeline.
* **Địa hình & Bản đồ:** `.ewld` (Terrain world container), `.eobj` (World object list), `.etex`/`.etexh` (Terrain texture).

---

## 2. Vị trí & Kiểm tra công cụ

### 2.1. Vị trí file thực thi
File thực thi đã được biên dịch sẵn tại:
```powershell
d:\PrototypeElionClientSrc\build\asset-tool\Release\ElionAssetTool.exe
```

Nếu cần build lại từ mã nguồn:
```powershell
powershell -File scripts/build-asset-tool.ps1 -VcpkgRoot <duong_dan_vcpkg>
```

### 2.2. Tự kiểm tra tính toàn vẹn (Self-Test)
Trước khi sử dụng, hãy chạy self-test để đảm bảo các thuật toán mã hóa, CRC32 và layout logic hoạt động chuẩn xác:
```powershell
.\build\asset-tool\Release\ElionAssetTool.exe self-test
```
*Kết quả hiển thị `self-test: passed` là công cụ sẵn sàng.*

---

## 3. Chi tiết các lệnh chuyển đổi theo nhóm tài nguyên

### 3.1. Chuyển đổi Mô hình 3D (`BMD` → `.emhd`, `.emdl`, `.eani`, `.egpu`)

MU gốc lưu tất cả thông tin trong một file `.bmd`. `convert-model` sẽ phân tích mesh, remap cây phân cấp xương, trích xuất animation và tạo buffer đồ họa tối ưu hóa cho Vulkan.

#### Cú pháp:
```powershell
ElionAssetTool.exe convert-model --input <file_bmd_goc> --output <base_path_khong_duoi> --texture-path <thu_muc_texture_elion/>
```

#### Tham số:
* `--input`: Đường dẫn file `.bmd` gốc (hỗ trợ plain BMD v10 và BMD v12 map-cipher).
* `--output`: Tên file đầu ra **không kèm phần mở rộng** (công cụ tự sinh 4 file: `.emhd`, `.emdl`, `.eani`, `.egpu`).
* `--texture-path`: Đường dẫn tương đối từ thư mục `data/` mà model sẽ tìm texture (bắt buộc kết thúc bằng dấu `/`, ví dụ `object74/`, `player/`, `item/`).

#### Ví dụ:
```powershell
ElionAssetTool.exe convert-model `
  --input "Mu Online ORG\Data\Object74\object01.bmd" `
  --output "build\stage\object01" `
  --texture-path "object74/"
```
*Kết quả:* Tạo ra 4 file `object01.emhd`, `object01.emdl`, `object01.eani`, `object01.egpu` trong thư mục `build\stage\`.

> [!WARNING]
> Một số file `.bmd` trong MU gốc là file rỗng giả lập (placeholder version 0) có kích thước đúng **42 bytes**. `convert-model` sẽ từ chối các file này. Khi viết script tự động, cần bỏ qua các file 42 bytes này.

---

### 3.2. Chuyển đổi Texture (`OZJ`, `OZT`, `OZB`, `JPG`, `BMP`, `TGA` → `.jtex`, `.ttex`)

Công cụ giải mã container ảnh Webzen (loại bỏ header 4 bytes hoặc nhận diện JPEG marker `FF D8`), chuyển đổi pixel sang `DXGI_FORMAT_R8G8B8A8_UNORM`, nén DDS RGBA8 lossless và mã hóa Blowfish.

#### Cú pháp:
```powershell
ElionAssetTool.exe convert-texture --input <anh_goc> --output <file_dich> [--sampler <0..3>]
```

#### Tham số:
* `--input`: File ảnh gốc (`.ozj`, `.ozt`, `.ozb`, `.jpg`, `.bmp`, `.tga`).
* `--output`: Đường dẫn đích, đuôi bắt buộc phải là `.jtex`, `.ttex` hoặc `.etex`.
* `--sampler` *(tùy chọn, mặc định: 2)*:
  * `0`: Nearest Clamp
  * `1`: Nearest Repeat
  * `2`: Linear Clamp (mặc định - phù hợp UI và model object)
  * `3`: Linear Repeat (phù hợp texture lặp mặt đất/địa hình)

#### Ví dụ:
```powershell
# Chuyển đổi OZJ (JPEG) sang .jtex
ElionAssetTool.exe convert-texture --input "Mu Online ORG\Data\Object74\tree01.ozj" --output "build\stage\tree01.jtex"

# Chuyển đổi OZT (TGA có kênh Alpha) sang .ttex
ElionAssetTool.exe convert-texture --input "Mu Online ORG\Data\Object74\leaf01.ozt" --output "build\stage\leaf01.ttex"
```
*Lưu ý:* Công cụ sẽ sinh ra cả cặp:
* `tree01.jtex` và `tree01.jtexh`
* `leaf01.ttex` và `leaf01.ttexh`

> [!IMPORTANT]
> Luôn phải sao chép cả file thân (`.jtex`/`.ttex`) lẫn file header (`.jtexh`/`.ttexh`) vào client. Nếu thiếu file `h`, game sẽ không thể khởi tạo texture và gây crash hoặc đen màn hình.

---

### 3.3. Chuyển đổi Cơ sở dữ liệu Game (Local Data → `.datx`)

Tất cả bảng dữ liệu gameplay của MU (mã hóa XOR BuxCode: `0xFC, 0xCF, 0xAB`) được chuẩn hóa sang `.datx` nén zlib và mã hóa Blowfish:

| Lệnh `ElionAssetTool` | File nguồn MU gốc | File đích Elion (`data/local/`) | Mục đích sử dụng |
|---|---|---|---|
| `convert-text` | `Local\Eng\Text_eng.bmd` | `Text.datx` | Chuỗi ký tự ngôn ngữ game |
| `convert-dialog` | `Local\Eng\Dialog_eng.bmd` | `Dialog.datx` | Nội dung hội thoại NPC |
| `convert-movereq` | `Local\Eng\movereq_eng.bmd` | `Movereq.datx` | Danh sách cổng dịch chuyển (Warp) |
| `convert-item` | `Local\Eng\item_eng.bmd` | `Item.datx` | Cơ sở dữ liệu vật phẩm gốc |
| `create-item-categories` | `Local\Eng\item_eng.bmd` | `itemcategoriesdb.datx` | **Bắt buộc startup:** Bảng phân loại item |
| `convert-itemsettype` | `Local\Eng\itemsettype.bmd` | `ItemSetType.datx` | Định danh phân loại Set đồ thần |
| `convert-itemsetoption`| `Local\Eng\itemsetoption_eng.bmd`| `ItemSetOption.datx`| Dòng chỉ số đồ thần |
| `convert-socketitem` | `Local\Eng\socketitem_eng.bmd` | `SocketItem.datx` | Thuộc tính vật phẩm ngọc socket |
| `convert-skill` | `Local\Eng\skill_eng.bmd` (input) + `Local\Skill.bmd` (base-input) | `Skill.datx` | Bảng thuộc tính kỹ năng nhân vật |
| `convert-buffeffect` | `Local\Eng\BuffEffect_eng.bmd` | `BuffEffect.datx` | Bảng hiệu ứng trạng thái (Buff/Debuff) |
| `convert-pet` | `Local\pet.bmd` | `Pet.datx` | Bảng chỉ số thú nuôi (Pet) |
| `convert-mix` | `Local\mix.bmd` | `Mix.datx` | Tỷ lệ và công thức Chaos Goblin |
| `convert-harmony` | `Local\Eng\JewelOfHarmonyOption_eng.bmd` | `JewelOfHarmonyOption.datx` | Thuộc tính ép đá tạo hóa Harmony |
| `convert-monsterskill` | `Local\MonsterSkill.bmd` | `MonsterSkill.datx` | Kỹ năng đặc biệt của Monster |
| `convert-quest` | `Local\Eng\Quest_eng.bmd` | `Quest.datx` | Cấu hình nhiệm vụ (Quest) |
| `convert-fixed-bux` | `gate.bmd` (`--record-size 14`) | `Gate.datx` | Tọa độ cổng dịch chuyển Map |
| `convert-fixed-bux` | `Local\ItemAddOption.bmd` (`--record-size 16`) | `ItemAddOption.datx` | Tùy chọn gia cường vật phẩm |
| `convert-fixed-bux-strings` | `Local\Filter.bmd` (`--record-size 20`) | `Filter.datx` | Danh sách từ khóa cấm |
| `convert-fixed-bux-strings` | `Local\FilterName.bmd` (`--record-size 10`) | `FilterName.datx` | Danh sách tên nhân vật cấm |

#### Ví dụ: Chuyển đổi Skill (yêu cầu 2 file nguồn kết hợp):
```powershell
ElionAssetTool.exe convert-skill `
  --input "Mu Online ORG\Data\Local\Eng\skill_eng.bmd" `
  --base-input "Mu Online ORG\Data\Local\Skill.bmd" `
  --output "build\stage\Skill.datx"
```

#### Ví dụ: Tạo `itemcategoriesdb.datx` bắt buộc cho startup:
```powershell
ElionAssetTool.exe create-item-categories `
  --item-input "Mu Online ORG\Data\Local\Eng\item_eng.bmd" `
  --output "build\stage\itemcategoriesdb.datx"
```

---

## 4. Các lệnh kiểm tra, thẩm định & phân tích

### 4.1. Thẩm định cây tài nguyên Elion (`verify-tree`)
Quét toàn bộ thư mục Elion Data, giải mã thử để xác thực CRC32, Blowfish và kiểm tra xem có texture nào bị mất file header sidecar không:
```powershell
ElionAssetTool.exe verify-tree --root "build\stage" --output "verify-report.json" --strict
```
*Cờ `--strict`: Nếu phát hiện bất kỳ file lỗi hoặc file lẻ loi nào sẽ trả về exit code khác 0 để dừng pipeline kịp thời.*

### 4.2. Xem thông số chi tiết của file BMD nguồn (`inspect-bmd`)
```powershell
ElionAssetTool.exe inspect-bmd --input "Mu Online ORG\Data\Object74\object01.bmd"
```
*Hiển thị số lượng mesh, bone, dummy bone, action frame, texture name.*

### 4.3. Quét kiểm tra toàn bộ cây BMD nguồn (`verify-bmd-tree`)
```powershell
ElionAssetTool.exe verify-bmd-tree --root "Mu Online ORG\Data" --output "bmd-report.json"
```
*Thống kê có bao nhiêu file BMD chuẩn plain v10/v12, bao nhiêu file phiên bản cũ/mã hóa không tương thích.*

### 4.4. Giải mã container Elion ra file raw (`decrypt`)
Dùng khi bạn muốn trích xuất dữ liệu gốc bên trong một file `.datx`, `.emdl`, `.jtex`:
```powershell
ElionAssetTool.exe decrypt --input "Elion Client\data\local\Text.datx" --output "Text_plain.bin" --domain data --decompress
```

### 4.5. Đóng gói mã hóa thủ công (`encrypt`)
Mã hóa một file thô thành container Elion chuẩn:
```powershell
ElionAssetTool.exe encrypt --input "MyData.bin" --output "MyData.datx" --domain data --signature DATX --compress
```

---

## 5. Kịch bản thực tế & Script tự động hóa

Trong thư mục `scripts/` của repo đã có sẵn các script PowerShell chuẩn hóa các quy trình chuyển đổi thường dùng:

### 5.1. Khôi phục toàn bộ bảng dữ liệu Local
Chạy script:
```powershell
powershell -File scripts/recover-local-data.ps1
```
*Script tự động convert toàn bộ các bảng trong `Local\Eng`, thẩm định tính toàn vẹn và copy vào `Elion Client\data\local`.*

### 5.2. Khôi phục toàn bộ Object74 (Login Scene)
Chạy script:
```powershell
powershell -File scripts/recover-world74.ps1
```
*Tự động convert 105 model và 107 texture của Object74 sang `.emhd/.emdl/.eani/.egpu` và `.jtex/.ttex`, kiểm tra và triển khai vào `Elion Client\data\object74`.*

### 5.3. Script mẫu chuyển đổi hàng loạt cho một thư mục bất kỳ
Dưới đây là mẫu script PowerShell để bạn convert một thư mục Object, Item hoặc Monster tùy ý:

```powershell
param(
    [string]$SourceFolder = "d:\PrototypeElionClientSrc\Mu Online ORG\Data\Object1",
    [string]$ElionTexturePath = "object1/",
    [string]$DestinationFolder = "d:\PrototypeElionClientSrc\Elion Client\data\object1"
)

$ErrorActionPreference = 'Stop'
$tool = "d:\PrototypeElionClientSrc\build\asset-tool\Release\ElionAssetTool.exe"
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$stage = "d:\PrototypeElionClientSrc\build\stage-$stamp"

New-Item -ItemType Directory -Force -Path $stage | Out-Null

Write-Host "1. Chuyển đổi Model 3D..."
Get-ChildItem -LiteralPath $SourceFolder -Filter "*.bmd" | ForEach-Object {
    $bytes = [IO.File]::ReadAllBytes($_.FullName)
    # Bỏ qua placeholder version 0 dài 42 bytes
    if ($bytes.Length -eq 42 -and $bytes[0] -eq 0x42 -and $bytes[1] -eq 0x4D -and $bytes[2] -eq 0x44) {
        Write-Host "Bo qua placeholder BMD: $($_.Name)"
        return
    }

    $baseOutput = Join-Path $stage $_.BaseName.ToLowerInvariant()
    & $tool convert-model --input $_.FullName --output $baseOutput --texture-path $ElionTexturePath
}

Write-Host "2. Chuyển đổi Texture..."
Get-ChildItem -LiteralPath $SourceFolder -File | Where-Object { $_.Extension -in '.OZJ', '.OZT', '.JPG', '.TGA' } | ForEach-Object {
    $ext = if ($_.Extension -ieq '.OZT' -or $_.Extension -ieq '.TGA') { '.ttex' } else { '.jtex' }
    $outPath = Join-Path $stage ($_.BaseName.ToLowerInvariant() + $ext)
    & $tool convert-texture --input $_.FullName --output $outPath
}

Write-Host "3. Thẩm định tính toàn vẹn (Verify)..."
$verifyReport = Join-Path $stage "verify-report.json"
& $tool verify-tree --root $stage --output $verifyReport --strict

Write-Host "4. Triển khai vào Client Data..."
New-Item -ItemType Directory -Force -Path $DestinationFolder | Out-Null
Get-ChildItem -LiteralPath $stage -File | Where-Object { $_.Name -ne "verify-report.json" } | Copy-Item -Destination $DestinationFolder -Force

Write-Host "Hoan tat chuyen doi! Da trien khai vao: $DestinationFolder"
```

---

## 6. Xử lý các sự cố thường gặp (Troubleshooting)

| Sự cố / Lỗi hiển thị | Nguyên nhân | Cách khắc phục |
|---|---|---|
| `refusing to overwrite model output: ...` | File output đã tồn tại trong thư mục đích | Xóa thư mục cũ hoặc tạo một thư mục staging mới với timestamp |
| `texture output or header already exists` | File `.jtex` hoặc `.jtexh` đã có sẵn | Đảm bảo thư mục đích là thư mục trống |
| `not a plain BMD v10/v12 file` | File BMD rỗng 42 bytes hoặc thuộc phiên bản MU season mới hơn có mã hóa khác | Bỏ qua các file 42 bytes; dùng lệnh `inspect-bmd` để kiểm tra header |
| `cannot decode Windows-1252 text` | File BMD ngôn ngữ chứa ký tự không tương thích bảng mã | Kiểm tra xem file BMD nguồn có đúng định dạng ANSI/Windows-1252 hay không |
| Game bị crash hoặc texture đen xì khi chạy | Thiếu file header `.jtexh` hoặc `.ttexh` | Luôn copy cả cặp file `.jtex` + `.jtexh`. Chạy lại `verify-tree --strict` để kiểm tra |
| Model không nhận texture | Tham số `--texture-path` sai | `--texture-path` phải đúng đường dẫn tương đối từ `data/` và kết thúc bằng `/` (ví dụ `object74/`) |
