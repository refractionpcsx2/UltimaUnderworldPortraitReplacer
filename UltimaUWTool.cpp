#include <iostream>
#include <stdio.h>
#include <string>
#include <filesystem>
#include <unordered_map>

#pragma pack(push, 1)

// 14 bytes
struct BITMAPFILEHEADER {
    uint16_t bfType;      // Must be 'BM' (0x4D42)
    uint32_t bfSize;      // Size of the entire file
    uint16_t bfReserved1; // Must be 0
    uint16_t bfReserved2; // Must be 0
    uint32_t bfOffBits;   // Offset to pixel data
};

// 40 bytes (most common DIB header)
struct BITMAPINFOHEADER {
    uint32_t biSize;          // Size of this header (40 bytes)
    int32_t  biWidth;         // Image width
    int32_t  biHeight;        // Image height (positive = bottom-up)
    uint16_t biPlanes;        // Must be 1
    uint16_t biBitCount;      // Bits per pixel (1, 4, 8, 16, 24, 32)
    uint32_t biCompression;   // Compression method
    uint32_t biSizeImage;     // Image size (may be 0 for BI_RGB)
    int32_t  biXPelsPerMeter; // Horizontal resolution
    int32_t  biYPelsPerMeter; // Vertical resolution
    uint32_t biClrUsed;       // Number of colors in palette
    uint32_t biClrImportant;  // Important colors
};

struct GraphicHeader
{
    uint8_t isForcedSize;
    uint8_t imageCount;
    uint8_t imageCount1;
};

struct GraphicFile
{
    std::string filename;
    int offset;
};

#pragma pack(pop)

uint16_t imageCount = 0;
uint32_t imageOffsets[256];
uint8_t* filebuffer;
uint8_t* imagebuffer;
uint8_t palette[256 * 3];
int32_t out_palette[256];

uint8_t data_offset = 5;

bool DumpFileRoutine(GraphicHeader& header, FILE* f, int palette_index, std::string folder_name)
{
    FILE* out_f;
    int file_index = 0;
    BITMAPFILEHEADER outfile_bitmap_header;
    BITMAPINFOHEADER outfile_bitmap_infohead;
    // Read out the files
    for (int i = 0; i < imageCount; i++)
    {
        uint32_t image_size = imageOffsets[i + 1] - imageOffsets[i];

        if (image_size > 0)
        {
            file_index++;
            //printf("Image %d offset 0x%05x\n", i, imageOffsets[i]);
            std::string offset_string(std::to_string(imageOffsets[i]));
            std::string prefix_folder(folder_name + "/");
            std::filesystem::create_directories(prefix_folder);
            std::string out_filename = std::string(prefix_folder + "graphic" + std::string((6 - offset_string.length()), '0') + offset_string + ".bmp");
            fopen_s(&out_f, out_filename.c_str(), "wb");
            fseek(f, imageOffsets[i], 0);


            filebuffer = static_cast<uint8_t*>(malloc(image_size));

            fread(filebuffer, 1, image_size, f);

            uint8_t storage_format = filebuffer[0];
            uint8_t width = filebuffer[1];
            uint8_t height = filebuffer[2];

            if (storage_format > 4 || width > 200 || height > 200)
            {
                printf("%s File corrupted\n", folder_name.c_str());
                free(filebuffer);
                return false;
            }
            uint16_t imgsize = width * height;

            uint32_t palette_size = (256 * 4);
            uint32_t output_file_size = 14 + 40 + palette_size;


            int offset = 0;
            int outsize = 0;
            int pos = 0;
            int start_pos = image_size;
            int pad = ((width + 3) & ~3) - width;

            imagebuffer = static_cast<uint8_t*>(malloc(imgsize + (pad * height))); // 100kb, should be more than enough for padding
            for (int j = start_pos - width; j >= data_offset; j++)
            {
                imagebuffer[offset++] = filebuffer[j];
                pos++;
                outsize++;
                if (pos == width)
                {
                    if (pad != 0)
                    {
                        for (int k = 0; k < pad; k++)
                        {
                            imagebuffer[offset++] = 0;
                            outsize++;
                        }
                    }
                    j -= width * 2;
                    pos = 0;
                }

            }
            free(filebuffer);

            // Process bitmap header
            outfile_bitmap_header.bfType = 'MB';
            outfile_bitmap_header.bfOffBits = 14 + 40 + (256 * 4);
            output_file_size += outsize;
            outfile_bitmap_header.bfSize = output_file_size;
            outfile_bitmap_header.bfReserved1 = 0;
            outfile_bitmap_header.bfReserved2 = 0;

            fwrite(&outfile_bitmap_header, 1, 14, out_f);

            outfile_bitmap_infohead.biSize = 40;
            outfile_bitmap_infohead.biWidth = width;
            outfile_bitmap_infohead.biHeight = height;
            outfile_bitmap_infohead.biPlanes = 1;
            outfile_bitmap_infohead.biBitCount = 8;
            outfile_bitmap_infohead.biCompression = 0;
            outfile_bitmap_infohead.biSizeImage = imgsize;
            outfile_bitmap_infohead.biXPelsPerMeter = 0;
            outfile_bitmap_infohead.biYPelsPerMeter = 0;
            outfile_bitmap_infohead.biClrUsed = 256;
            outfile_bitmap_infohead.biClrImportant = 0;

            fwrite(&outfile_bitmap_infohead, 1, 40, out_f);
            fwrite(out_palette, 4, 256, out_f);

            fwrite(imagebuffer, 1, outsize, out_f);

            free(imagebuffer);
            fclose(out_f);

        }
    }
    
    std::string prefix_folder(folder_name + "/");
    std::string out_filename = std::string(prefix_folder + "palette" + std::to_string(palette_index) + ".bmp");
    fopen_s(&out_f, out_filename.c_str(), "wb");

    uint32_t palette_size = (256 * 4);
    uint32_t palette_output_size = 14 + 40 + palette_size;

    imagebuffer = static_cast<uint8_t*>(malloc(256)); // 100kb, should be emore than enough for padding
    for (int j = 0; j < 256; j++)
    {
        imagebuffer[j] = j;
    }

    // Process bitmap header
    outfile_bitmap_header.bfType = 'MB';
    outfile_bitmap_header.bfOffBits = palette_output_size;
    palette_output_size += 256;
    outfile_bitmap_header.bfSize = palette_output_size;
    outfile_bitmap_header.bfReserved1 = 0;
    outfile_bitmap_header.bfReserved2 = 0;

    fwrite(&outfile_bitmap_header, 1, 14, out_f);

    outfile_bitmap_infohead.biSize = 40;
    outfile_bitmap_infohead.biWidth = 16;
    outfile_bitmap_infohead.biHeight = 16;
    outfile_bitmap_infohead.biPlanes = 1;
    outfile_bitmap_infohead.biBitCount = 8;
    outfile_bitmap_infohead.biCompression = 0;
    outfile_bitmap_infohead.biSizeImage = 256;
    outfile_bitmap_infohead.biXPelsPerMeter = 0;
    outfile_bitmap_infohead.biYPelsPerMeter = 0;
    outfile_bitmap_infohead.biClrUsed = 256;
    outfile_bitmap_infohead.biClrImportant = 0;

    fwrite(&outfile_bitmap_infohead, 1, 40, out_f);
    fwrite(out_palette, 4, 256, out_f);

    fwrite(imagebuffer, 1, 256, out_f);

    fclose(out_f);

    free(imagebuffer);
    return true;
}
void DumpFiles()
{
    FILE* f;
    FILE* pal_f;

    GraphicHeader header;
    printf("Starting Portrait Dump\n");

   

    if (fopen_s(&pal_f, "PALS.DAT", "rb"))
    {
        printf("Failed to open palette file\n");

        return;
    }
    rewind(pal_f);
    fread(palette, 3, 256, pal_f);
    fclose(pal_f);

    for (int i = 0; i < 256; i++)
    {
        out_palette[i] = (palette[(i * 3) + 2] << 2) | ((palette[(i * 3) + 1] << 10)) | (palette[(i * 3)] << 18);
    }
    int palette_index = 0;

    if (fopen_s(&f, "HEADS.GR", "rb"))
    {
        printf("Failed to open heads file\n");

        return;
    }
    rewind(f);
    fread(&header, 1, 3, f);

    // Fix up the count variable (reading is a pain because of the offset)
    imageCount = std::min(256, header.imageCount | (header.imageCount1 << 8));
    // Read image offsets
    fread(&imageOffsets, 4, imageCount + 1, f);
    //printf("Forced Size? %s\n", (header.isForcedSize == 1) ? "No" : "Yes");
    //printf("Number of images: %d\n", imageCount);
    //printf("Image Offsets\n");
    if (!DumpFileRoutine(header, f, palette_index, std::string("heads")))
    {
        fclose(f);
        return;
    }
    
    fclose(f);

    if (fopen_s(&f, "BODIES.GR", "rb"))
    {
        printf("Failed to open char head file\n");

        return;
    }
    rewind(f);
    fread(&header, 1, 3, f);

    // Fix up the count variable (reading is a pain because of the offset)
    imageCount = std::min(256, header.imageCount | (header.imageCount1 << 8));
    // Read image offsets
    fread(&imageOffsets, 4, imageCount + 1, f);
    //printf("Forced Size? %s\n", (header.isForcedSize == 1) ? "No" : "Yes");
    //printf("Number of images: %d\n", imageCount);
    //printf("Image Offsets\n");
    if (!DumpFileRoutine(header, f, palette_index, std::string("bodies")))
    {
        fclose(f);
        return;
    }

    fclose(f);

    if (fopen_s(&f, "CHRBTNS.GR", "rb"))
    {
        printf("Failed to open char head file\n");

        return;
    }
    rewind(f);
    fread(&header, 1, 3, f);

    // Fix up the count variable (reading is a pain because of the offset)
    imageCount = std::min(256, header.imageCount | (header.imageCount1 << 8));
    // Read image offsets
    fread(&imageOffsets, 4, imageCount + 1, f);

    if (fopen_s(&pal_f, "PALS.DAT", "rb"))
    {
        printf("Failed to open palette file\n");

        return;
    }
    fseek(pal_f, 256 * 3 * 3, 0);
    fread(palette, 3, 256, pal_f);
    fclose(pal_f);

    for (int i = 0; i < 256; i++)
    {
        out_palette[i] = (palette[(i * 3) + 2] << 2) | ((palette[(i * 3) + 1] << 10)) | (palette[(i * 3)] << 18);
    }
    palette_index = 3;

    //printf("Forced Size? %s\n", (header.isForcedSize == 1) ? "No" : "Yes");
    //printf("Number of images: %d\n", imageCount);
    //printf("Image Offsets\n");
    if (!DumpFileRoutine(header, f, palette_index, std::string("chrbtns")))
    {
        fclose(f);
        return;
    }

    fclose(f);
    
    // printf("File end at offset 0x%05x\n", imageOffsets[imageCount]);

    printf("Finished Exporting images\n");
    printf("-------------------------\n");
    printf("You can now replace the portrait with your custom one.\n");
    printf("Be sure to use BMP format with the dimensions match the original file you are replacing!.\n");
    printf("Available colours have been dumped to palette.bmp if you wish to match the game's colours, else it will pick nearest colour\n");
}

bool ImportRoutine(int heads_size, std::string directory)
{
    FILE* in_f;
    int file_index = 0;

    std::vector<GraphicFile> files;
    std::filesystem::path dir = std::filesystem::current_path() / directory;

    if (!std::filesystem::exists(dir))
    {
        printf("No heads files found, did you export them?\n");
        return false;
    }
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        std::string path(entry.path().string());
        if (entry.path().string().find("graphic") != std::string::npos && entry.path().string().find(".bmp") != std::string::npos)
        {
            int offset = std::stoi(path.substr(path.length() - 9, 6).c_str());
            files.push_back(GraphicFile(entry.path().string(), offset));
        }
    }
    if (files.size() == 0)
    {
        printf("No files found, did you export them?\n");
        return false;
    }

    std::vector<GraphicFile>::iterator it;
    for (it = files.begin(); it != files.end(); ++it)
    {
        printf("File: %s Offset %05x\n", it[0].filename.c_str(), it[0].offset);

        errno_t err = fopen_s(&in_f, it[0].filename.c_str(), "rb");
        if (err != 0)
        {
            printf("Failed to bmp file ERR %d\n", err);

            return false;
        }
        fseek(in_f, 0, SEEK_END);

        int file_size = ftell(in_f);
        imagebuffer = static_cast<uint8_t*>(malloc(file_size));
        rewind(in_f);
        fread(imagebuffer, 1, file_size, in_f);

        BITMAPFILEHEADER outfile_bitmap_header;
        BITMAPINFOHEADER outfile_bitmap_infohead;

        rewind(in_f);
        fread(&outfile_bitmap_header, 1, 14, in_f);
        fseek(in_f, 14, 0);
        fread(&outfile_bitmap_infohead, 1, 40, in_f);

        uint16_t bitDepth = outfile_bitmap_infohead.biBitCount;

        int width = outfile_bitmap_infohead.biWidth;
        int height = outfile_bitmap_infohead.biHeight;
        int pixel_index = 0;

       // printf("Format %d", bitDepth);
        if (bitDepth == 24)
        {
            file_index++;
            int total_size = (width * height);
            pixel_index = total_size - (width - 1);
            int rowSize = ((width * 3 + 3) & ~3);
            int pad = rowSize - (width * 3);
            int padded_width = width + pad;
            int offset = (outfile_bitmap_header.bfSize - outfile_bitmap_header.bfOffBits);
            bool new_row = false;
            int pos = 0;
            for (int i = 0; i < offset; i += 3) {
                if (pos == width)
                {
                    i += pad;
                    new_row = true;
                    pos = 0;
                    if (i > offset - (width * 3))
                        break;
                }
                const int32_t cur_pixel_r = imagebuffer[outfile_bitmap_header.bfOffBits + i + 0];
                const int32_t cur_pixel_g = imagebuffer[outfile_bitmap_header.bfOffBits + i + 1];
                const int32_t cur_pixel_b = imagebuffer[outfile_bitmap_header.bfOffBits + i + 2];

                int palette_entry = 0;
                int best_match_value = INT_MAX;

                // This could technically improve the accuracy but it seems fine without it.
                int red_multi = 1;//std::max(((cur_pixel_r > cur_pixel_g) ? 2 : 0) + ((cur_pixel_r > cur_pixel_b) ? 2 : 0), 1);
                int blue_multi = 1;//std::max(((cur_pixel_b > cur_pixel_g) ? 2 : 0) + ((cur_pixel_b > cur_pixel_r) ? 2 : 0), 1);
                int green_multi = 1;//std::max(((cur_pixel_g > cur_pixel_b) ? 2 : 0) + ((cur_pixel_g > cur_pixel_r) ? 2 : 0), 1);

                for (int j = 0; j < 256; j++)
                {
                    int dist_r = (cur_pixel_r - (out_palette[j] & 0xFF));
                    int dist_g = (cur_pixel_g - ((out_palette[j] >> 8) & 0xFF));
                    int dist_b = (cur_pixel_b - ((out_palette[j] >> 16) & 0xFF));

                    int distance = (red_multi * dist_r * dist_r) + (green_multi * dist_g * dist_g) + (blue_multi * dist_b * dist_b);
                    if (distance < best_match_value)
                    {
                        best_match_value = distance;
                        palette_entry = j;

                        if (distance == 0)
                            break;
                    }
                }

                if (new_row)
                {
                    pixel_index -= width * 2;
                    new_row = false;
                }

                if ((it[0].offset + 4 + pixel_index) > heads_size)
                {
                    printf("Buffer overflow");
                    fclose(in_f);
                    return false;
                }
                filebuffer[it[0].offset + 4 + pixel_index++] = palette_entry;
                pos++;
            }
        }
        else if (bitDepth == 8)
        {
            uint32_t file_palette[256];
            fseek(in_f, 40 + 14, 0);
            fread(&file_palette, 4, 256, in_f);

            file_index++;
            int total_size = (width * height);
            pixel_index = total_size - (width - 1);
            int rowSize = ((width + 3) & ~3);
            int pad = rowSize - width;
            int padded_width = width + pad;
            int offset = (total_size + (pad * height));
            bool new_row = false;
            int pos = 0;
            for (int i = 0; i < offset; i++) {
                if (pos == width)
                {
                    i += pad;
                    new_row = true;
                    pos = 0;
                    if (i >= offset)
                        break;
                }
                const int32_t cur_pixel_index =imagebuffer[outfile_bitmap_header.bfOffBits + i];
                const int32_t cur_pixel_r = file_palette[cur_pixel_index] & 0xFF;
                const int32_t cur_pixel_g = (file_palette[cur_pixel_index] >> 8) & 0xFF;
                const int32_t cur_pixel_b = (file_palette[cur_pixel_index] >> 16) & 0xFF;

                int palette_entry = 0;
                int best_match_value = INT_MAX;

                // This could technically improve the accuracy but it seems fine without it.
                int red_multi = 1;//std::max(((cur_pixel_r > cur_pixel_g) ? 2 : 0) + ((cur_pixel_r > cur_pixel_b) ? 2 : 0), 1);
                int blue_multi = 1;//std::max(((cur_pixel_b > cur_pixel_g) ? 2 : 0) + ((cur_pixel_b > cur_pixel_r) ? 2 : 0), 1);
                int green_multi = 1;//std::max(((cur_pixel_g > cur_pixel_b) ? 2 : 0) + ((cur_pixel_g > cur_pixel_r) ? 2 : 0), 1);

                for (int j = 0; j < 256; j++)
                {
                    int dist_r = (cur_pixel_r - (out_palette[j] & 0xFF));
                    int dist_g = (cur_pixel_g - ((out_palette[j] >> 8) & 0xFF));
                    int dist_b = (cur_pixel_b - ((out_palette[j] >> 16) & 0xFF));

                    int distance = (red_multi * dist_r * dist_r) + (green_multi * dist_g * dist_g) + (blue_multi * dist_b * dist_b);
                    if (distance < best_match_value)
                    {
                        best_match_value = distance;
                        palette_entry = j;

                        if (distance == 0)
                            break;
                    }
                }

                if (new_row)
                {
                    pixel_index -= width * 2;
                    new_row = false;
                }
                if ((it[0].offset + 4 + pixel_index) > heads_size)
                {
                    printf("Buffer overflow");
                    fclose(in_f);
                    return false;
                }
                filebuffer[it[0].offset + 4 + pixel_index++] = palette_entry;
                pos++;
            }
        }
        fclose(in_f);
        free(imagebuffer);
    }
    return true;
}

void ImportFiles()
{
    FILE* f;
    FILE* pal_f;

    if (fopen_s(&f, "BODIES.GR", "rb+"))
    {
        printf("Failed to open bodies file for reading\n");

        return;
    }

    fseek(f, 0, SEEK_END);
    int heads_size = ftell(f);
    filebuffer = static_cast<uint8_t*>(malloc(heads_size));

    rewind(f);
    fread(filebuffer, 1, heads_size, f);
    fclose(f);

    if (fopen_s(&pal_f, "PALS.DAT", "rb"))
    {
        printf("Failed to open palette file\n");

        return;
    }
    rewind(pal_f);
    fread(palette, 3, 256, pal_f);
    fclose(pal_f);

    for (int i = 0; i < 256; i++)
    {
        out_palette[i] = (palette[(i * 3) + 2] << 2) | ((palette[(i * 3) + 1] << 10)) | (palette[(i * 3)] << 18);
    }
    if (!ImportRoutine(heads_size, "bodies"))
        return;

    if (fopen_s(&f, "BODIES.GR", "wb"))
    {
        printf("Failed to open char head file\n");

        return;
    }
    rewind(f);
    fwrite(filebuffer, 1, heads_size, f);
    fclose(f);

    free(filebuffer);

    if (fopen_s(&f, "HEADS.GR", "rb+"))
    {
        printf("Failed to open char head file for reading\n");

        return;
    }

    fseek(f, 0, SEEK_END);
    heads_size = ftell(f);
    filebuffer = static_cast<uint8_t*>(malloc(heads_size));

    rewind(f);
    fread(filebuffer, 1, heads_size, f);
    fclose(f);

    if (!ImportRoutine(heads_size, "heads"))
        return;

    if (fopen_s(&f, "HEADS.GR", "wb"))
    {
        printf("Failed to open char head file\n");

        return;
    }
    rewind(f);
    fwrite(filebuffer, 1, heads_size, f);
    fclose(f);

    free(filebuffer);

    if (fopen_s(&f, "CHRBTNS.GR", "rb+"))
    {
        printf("Failed to open char head file for reading\n");

        return;
    }

    fseek(f, 0, SEEK_END);
    heads_size = ftell(f);
    filebuffer = static_cast<uint8_t*>(malloc(heads_size));

    rewind(f);
    fread(filebuffer, 1, heads_size, f);
    fclose(f);

    if (fopen_s(&pal_f, "PALS.DAT", "rb"))
    {
        printf("Failed to open palette file\n");

        return;
    }
    fseek(pal_f, 256 * 3 * 3, 0);
    fread(palette, 3, 256, pal_f);
    fclose(pal_f);

    for (int i = 0; i < 256; i++)
    {
        out_palette[i] = (palette[(i * 3) + 2] << 2) | ((palette[(i * 3) + 1] << 10)) | (palette[(i * 3)] << 18);
    }

    if (!ImportRoutine(heads_size, "chrbtns"))
        return;

    if (fopen_s(&f, "CHRBTNS.GR", "wb"))
    {
        printf("Failed to open char head file\n");

        return;
    }
    rewind(f);
    fwrite(filebuffer, 1, heads_size, f);
    fclose(f);

    free(filebuffer);

    printf("Finished Importing images\n");
    printf("-------------------------\n");
    printf("You now need to copy the BODIES.GR, HEADS.GR and CHRBTNS.GR back in to your Ultima Underworld 1 or 2 folder\n");

    return;
}

int main(int argc, char* argv[])
{
    printf("Ultima Underworld 1 & 2 Portrait Replacer Version 0.2\n");

    if (argc > 1)
    {
        if (!std::strcmp(argv[1], "export"))
            DumpFiles();
        else if (!std::strcmp(argv[1], "import"))
            ImportFiles();
        else
        {
            printf("No mode selected\n");
            printf("You must either provide 'export' or 'import' as an argument\n");
        }
    }
    else
    {
        printf("No mode selected\n");
        printf("You must either provide 'export' or 'import' as an argument\n");
    }

    printf("Press enter to exit\n");
    getchar();
    
    return 0;
}
