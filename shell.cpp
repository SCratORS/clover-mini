#include "Shell.h"
#include <cstdio>
#include <string>
#include <iostream>
#include <filesystem>

/*
uint16_t RGB888ToRGB565( uint32_t color )
{
	return (color>>8&0xf800)|(color>>5&0x07e0)|(color>>3&0x001f);
}
*/


namespace fs = std::filesystem;

uint8_t sp_index = 0;
uint64_t clock_counter;

bool Shell::loadWav(WavFile * wavFile, const char* fname) {
	#ifdef INFO
	printf("INFO: Load WAV File: %s\n", fname);
	#endif
	wavFile->file = fopen(fname, "rb");
	if (wavFile->file) {
		wavFile->offset = sizeof(wavFile->header);
		fread(&wavFile->header, sizeof(wavFile->header), 1, wavFile->file);
		if (wavFile->header.chunkID == 0x46464952) {
			#ifdef INFO
			printf("INFO: %s loaded.\n", fname);
			#endif
			fseek(wavFile->file , 0 , SEEK_END);
			return true;
		}	else {
			#ifdef INFO
			printf("ERROR: Incorrect WAV File: %s\n", fname);
			#endif
			delete wavFile;
			wavFile = NULL;
		}
	} else {
		#ifdef INFO
		printf("ERROR: Can't open file: %s\n", fname);
		#endif
		delete wavFile;
		wavFile = NULL;
	}
	return false;
}

Shell::Shell() {
	fadeProcents = 100;
}

void Shell::loadBitmap(const char* fname, Image * image) {
	FILE* fb = fopen(fname, "rb");
	if (fb) {
		BmpHeader header;
		BmpInfoHeader bmpInfo;
		char sig[2];
		fread(&sig, sizeof(sig), 1, fb);
		if (sig[0] == 'B' && sig[1] == 'M') {
			fread(&header, sizeof(header), 1, fb);
			fread(&bmpInfo, sizeof(bmpInfo), 1, fb);
			if (bmpInfo.numberOfColorPlanes == 1) {
				if (bmpInfo.colorDepth == 24 && bmpInfo.compressionMethod == 0) {
					image->width = bmpInfo.width;
					image->height = bmpInfo.height;
					uint8_t skip = bmpInfo.width%4;
					uint32_t sizeRaw = bmpInfo.width * bmpInfo.height;
					image->rawData.resize(sizeRaw);
					fseek(fb , header.pixelDataOffset, SEEK_SET);
					for (uint32_t i = 1; i <= bmpInfo.height; i++) {
						fread(image->rawData.data() + (bmpInfo.height-i) * bmpInfo.width, sizeof(BmpPixel), bmpInfo.width, fb);
						fseek(fb , skip, SEEK_CUR);
					}
					#ifdef INFO
					printf("INFO: %s loaded.\n", fname);
					#endif
				} else {
					#ifdef INFO
					printf("ERROR: Not supported BMP File: %s\n", fname);
					#endif
				}
			} else {
				#ifdef INFO
				printf("ERROR: Incorrect BMP File: %s\n", fname);
				#endif
			}
		} else {
			#ifdef INFO
			printf("ERROR: Not BMP file: %s\n", fname);
			#endif
		}
		fclose(fb);
	} else {
		#ifdef INFO
		printf("ERROR: Can't open file: %s\n", fname);
		#endif
	}
}

bool Shell::LoadData() {
	#ifdef INFO
	printf("INFO: %s\n", "Loading data.");
	#endif
	bgm_boot = new WavFile(); loadWav(bgm_boot, bgm_boot_snd);
	bgm_home = new WavFile(); loadWav(bgm_home, bgm_home_snd);
	se_sys_cursor = new WavFile(); loadWav(se_sys_cursor, se_sys_cursor_snd);
	se_sys_click_game = new WavFile(); loadWav(se_sys_click_game, se_sys_click_game_snd);
	se_sys_click = new WavFile(); loadWav(se_sys_click, se_sys_click_snd);
	se_sys_cancel = new WavFile(); loadWav(se_sys_cancel, se_sys_cancel_snd);
	se_sys_smoke = new WavFile(); loadWav(se_sys_smoke, se_sys_smoke_snd);
	PlayWav(bgm_boot);

	uint16_t index = 0;
 	for (const auto & entry : fs::directory_iterator(game_dir))
 		if (entry.is_directory())
	 		for (const auto & file : fs::directory_iterator(entry))
	 			if (fs::is_regular_file(file) && file.path().extension().string() == ".desktop") {
	 				INIReader reader(file.path().string().c_str());
	 				if (reader.ParseError() < 0) {
	 					#ifdef INFO
	 					printf("ERROR: Can't load '%s'\n", file.path().string().c_str());
	 					#endif
	 					continue;
	 				}
					card * Card = new card;	
					std::string icon = reader.Get("Desktop Entry", "Icon", "");
					Card->exec = reader.Get("Desktop Entry", "Exec", "");
					Card->name = reader.Get("Desktop Entry", "Name", "Unknown");
					Card->date = reader.Get("X-CLOVER Game", "ReleaseDate", "1970-01-01");
					Card->players = reader.GetInteger("X-CLOVER Game", "Players", 1);
					Card->simultaneous = reader.GetInteger("X-CLOVER Game", "Simultaneous", 0);

					Card->image = new Image();
					loadBitmap(icon.c_str(), Card->image);
					Card->image->x = 3+((73 - Card->image->width) >> 1);
					Card->image->y = 3+((69 - Card->image->height) >> 1);

					int16_t pos_e = icon.rfind(".");
					std::string ext = icon;
					if (pos_e>0) {
						ext.erase(0, pos_e);
	 					icon.erase(pos_e, icon.length() - pos_e);
	 				} else ext = "";
	 				icon.append("_small"+ext);

					navcard * NavCard = new navcard;
					NavCard->image = new Image();
					loadBitmap(icon.c_str(), NavCard->image);

					NavCard->y_pos = 172 + (20-NavCard->image->height);
					NavCard->x_pos = max_length_navbar;
					max_length_navbar += NavCard->image->width+1;

					Card->id = index++;
					Card->select = false;
					Card->pSelect = new bool;
					NavCard->id = Card->id;
					NavCard->select = Card->pSelect;
					courusel.push_back(Card);
					navigate.push_back(NavCard); 
	 			}

	uint16_t size = courusel.size();
 	if (size > 0 && size * 10 <= TBL_WIDTH ) {
	 	uint16_t count = TBL_WIDTH / (size*10);
	 	for (uint16_t i = 0; i < count; i++ ) {
	 		for (uint16_t c = 0; c < size; c++ ) {
	 			card * Card = new card;
	 			Card->pSelect = courusel[c]->pSelect;
	 			Card->image = courusel[c]->image;
	 			Card->exec = courusel[c]->exec;
	 			Card->name = courusel[c]->name;
	 			Card->date = courusel[c]->date;
	 			Card->players = courusel[c]->players;
	 			Card->simultaneous = courusel[c]->simultaneous;
	 			courusel.push_back(Card);
	 		}
	 	}
 	}
 	navigate_offset = (SCREEN_SIZE - max_length_navbar) << 1;
 	if (courusel.size() == 0) currentSelect = empty;
 	FILE* fp = fopen(table_sprites, "rb");
    if (!fp) return false;
    fread(&pallete[0], 0x200, 1, fp);
    fread(&sprites[0], 0x2000, 1, fp);
    fclose(fp);
	return true;
}

uint16_t sel = 0;
uint8_t l_y = 7;
void Shell::drawCourusel(uint8_t y, uint8_t counter) {
	uint8_t h = 10;
	uint8_t w = 10;
	max_length = courusel.size() * (w + 1);
	if (max_length < TBL_WIDTH+w+1) max_length = TBL_WIDTH+w+1;
	if (counter == 0) x_offset = (courusel_direction==0?x_offset:(courusel_direction<0?--x_offset:++x_offset))%max_length;
	int16_t x = x_offset;
	for (uint8_t i = 0; i < courusel.size(); i++) {
		courusel[i]->x_pos = ((i*(w+1)) + x) << 3;
		courusel[i]->y_pos = y << 3;
		int16_t bxs = (i*(w+1)) + x;
		int16_t bs = bxs;
		if (bs < 0) bs += max_length;
		else if (bs > max_length-1) bs -= max_length;
		if (bs == select_to) courusel_direction = 0;
		courusel[i]->select = bs == stable_position;
		* courusel[i]->pSelect = false;
		if (courusel[i]->select) {
			sel = i;
			if (courusel[i]->name.length() > TBL_WIDTH - 10) {
				uint16_t l = (courusel[i]->name.length() - (TBL_WIDTH - 10));
				courusel[i]->name.erase(courusel[i]->name.length() - l - 3, l + 3);
				courusel[i]->name += "...";
			}
			uint16_t offset_name = ((TBL_WIDTH - courusel[i]->name.length() - 10) >> 1) + 5;
			memcpy(&tblName[(l_y+1)*TBL_WIDTH+offset_name], courusel[i]->name.c_str(), courusel[i]->name.length());
			memset(&tblAttribute[(l_y+1)*TBL_WIDTH+offset_name], 0x37, courusel[i]->name.length());
		}
		for (uint8_t c = 0; c < w; c++) {
			int16_t bx = bxs + c;
			if (bx < 0) bx += max_length;
			else if (bx > max_length-1) bx -= max_length;
			if (bx > TBL_WIDTH-1) continue;
			int16_t bn = y * TBL_WIDTH + bx; 
			tblName[bn] = c==0?0x3B:(c==w-1?0x3D:0x3C);
			tblAttribute[bn] = courusel[i]->select?0x09:0x11;
			for (uint8_t v = 1; v < h; v++) {
				tblName[bn+v*TBL_WIDTH] = c==0?0x5B:(c==w-1?0x5C:(c==w-3&&v==h-1?(courusel[i]->players==1?0x8D:(courusel[i]->simultaneous==1?0x9D:0x8C)):(c==w-2&&v==h-1?(courusel[i]->simultaneous==1?0x9E:0x8E):(v==h-1&&c>0&&c<5?0x8F:0x4B))));
				tblAttribute[bn+v*TBL_WIDTH] = v==(h-1)?(courusel[i]->select?0x05:0x0D):(courusel[i]->select?0x09:0x11);
			
			}
			tblName[bn+h*TBL_WIDTH] = c==0?0x5A:(c==w-1?0x4D:0x4C);
			tblAttribute[bn+h*TBL_WIDTH] = courusel[i]->select?0x05:0x0D;
		}
	}
	if (courusel[sel]) * courusel[sel]->pSelect = true;
}

int16_t current_pos_x = -100;
int16_t current_pos_y = -100;

void Shell::drawNavigate(uint8_t counter) {
	if (!(currentSelect == gamelist || currentSelect == menu)) return;
	if (!counter) cursor = (++cursor)%3;
	if (max_length_navbar > SCREEN_SIZE-4) {
		navigate_offset = 0 - ((float)(max_length_navbar - SCREEN_SIZE + 4 ) / navigate.size()) * sel;		
		int16_t length_offset = navigate[sel]->x_pos + navigate_offset + navigate[sel]->image->width+1;		
		if (length_offset > SCREEN_SIZE-2)
		navigate_offset -= length_offset - SCREEN_SIZE  + 2;
	}
	current_pos_x = navigate[sel]->x_pos + navigate_offset + (navigate[sel]->image->width >> 1) - 8;
	current_pos_y = navigate[sel]->y_pos;
	for (uint8_t i = 0; i<2; i++) drawSprite(0x0A + cursor, current_pos_x + (i<<3), current_pos_y - 8, !i?0x82:0x02);
}

void Shell::drawSprite(uint8_t id, int16_t x, int16_t y, uint8_t attribute, uint8_t leng, uint8_t width, uint8_t opacity) {
	if (width == 0) width = leng;
	for (uint8_t i = 0; i < leng*width; i++ ) {
		uint8_t i1 = i/leng; uint8_t i2 = i%leng;
		if (attribute&0x40) OAM[sp_index].y = y - (i1<<3);
		else				OAM[sp_index].y = y + (i1<<3);
		if (attribute&0x80) OAM[sp_index].x = x - (i2<<3);
		else 				OAM[sp_index].x = x + (i2<<3);
		OAM[sp_index].attribute = attribute;
		OAM[sp_index].id = id + (i1<<4) + i2;
		OAM[sp_index].opacity = opacity;
		sp_index++;
	}
}

void Shell::drawMenu() {
	uint16_t start_position =  (SCREEN_SIZE>>1)-60;
	for (uint8_t i =0; i<4; i++) drawSprite(0x60+i*3, start_position+(i<<5), 8, i==3?0x0B:0x07, 3);
}

void Shell::drawTopBar() {
	for (uint8_t i = 0; i < 4; i++) {
		memset(&tblName[i*TBL_WIDTH], 0x10*i, TBL_WIDTH);
		if (i) memset(&tblAttribute[i*TBL_WIDTH + (HALF_TBL_WIDTH) - 8], 0x04, 16);
		else memset(&tblAttribute[i*TBL_WIDTH], 0x00, (TBL_WIDTH<<2));
	}
}

void Shell::drawDownBar() {
	for (uint8_t i = 0; i < 4; i++) memset(&tblName[(i+26)*TBL_WIDTH], (i==1?0x11:0x10) + (i<<4), TBL_WIDTH);
	memset(&tblAttribute[26*TBL_WIDTH], 0x08, TBL_WIDTH<<2);
	for (uint8_t i = 0; i<TBL_WIDTH; i++) {
		if ((i > HALF_TBL_WIDTH - 8) && (i < HALF_TBL_WIDTH + 7)) {
			uint16_t b = i - HALF_TBL_WIDTH + 7;
			tblName[26*TBL_WIDTH + i] = 0x12 + b;
			tblName[27*TBL_WIDTH + i] = 0x22 + b;
		} else {
			if ((i > TBL_WIDTH-9) && (i < TBL_WIDTH-1)) tblName[27*TBL_WIDTH+i] = 0x32 + (i - TBL_WIDTH + 8);
			else tblName[27*TBL_WIDTH + i] = 0x21;
		}
	}
}

int8_t animateboom = 0;
uint8_t animateboom_y = 0;
void Shell::drawTempSaveState(uint16_t counter) {
	if (!temp_save_state) return;
	uint8_t size = 2;
	uint16_t x = 20;
	uint8_t y = 176;
	uint8_t attr = 0x18;
	uint8_t opacity = 100;
	uint16_t grad = (counter%180) << 1;
	float rad = ((grad)*3.14) / 180.0;
	
	if (courusel[sel]->id == temp_save_state->id) size = 3;

	if (currentSelect == saves && size == 3) {
		size = 4;
		y = 104;
		x = 47 + 64 * select_save_state;
		attr = 0x09;
	}
	if (size == 2) {
		x += 4;
		y += 4;
		opacity = 70;
	}

	y += sin(rad) * 6;

	if (select_save_state < 0) {
		y -= animateboom*2;
		x += 4;
		if (animateboom == 0) drawSprite(0xCA, x, y, 0x11, 3, 0, opacity);
		if (animateboom == 1) drawSprite(0xCD, x, y, 0x11, 3, 0, opacity);
		if (animateboom == 2) drawSprite(0xB7, x, y, 0x11, 3, 0, opacity);
		if (animateboom == 3) {
			drawSprite(0xB2, x, y, 0x11, 3, 1, opacity);
			drawSprite(0xB5, x+8, y+8, 0x11, 2, 1, opacity);
		}
		if (animateboom == 4) {
			delete temp_save_state->image;
			delete temp_save_state;
			temp_save_state->image = nullptr;
			temp_save_state = nullptr;
			animateboom = 0;
			select_save_state = 0;
		}
		if (counter%4 == 0) animateboom++;
		return;
	}

	temp_save_state_y = y;
	temp_save_state_x = x;

	drawSprite(0x3B, x, 	y, attr, 1, 0, opacity);
	for (uint8_t i = 1; i <= size; i++) drawSprite(0x3C, x+8*i,	y, attr, 1, 0, opacity);
	drawSprite(0x3D, x+8+size*8, 	y, attr, 1, 0, opacity);
	
	for (uint8_t w = 1; w <= size-1; w++) {
		drawSprite(0x5B, x, 	y+8*w, attr, 1, 0, opacity);
		for (uint8_t i = 1; i <= size; i++) drawSprite(0x4B, x+8*i,	y+8*w, attr, 1, 0, opacity);
		drawSprite(0x5C, x+8+size*8, y+8*w, attr, 1, 0, opacity);
	}

	drawSprite(0x5A, x, 	y+size*8, attr, 1, 0, opacity);
	for (uint8_t i = 1; i <= size; i++) drawSprite(0x4C, x+8*i,	y+size*8, attr, 1, 0, opacity);
	drawSprite(0x4D, x+8+size*8, 	y+size*8, attr, 1, 0, opacity);

	OAM[sp_index].x = temp_save_state_x;
	OAM[sp_index].y = temp_save_state_y;
	OAM[sp_index].image = temp_save_state->image;
	OAM[sp_index].opacity = opacity;
	if (size == 2) {
		OAM[sp_index].image->x = 4;
		OAM[sp_index].image->crop_width = 23;
		OAM[sp_index].image->height = 15;
	} else if (size == 3) {
		OAM[sp_index].image->x = 4;
		OAM[sp_index].image->crop_width = 31;
		OAM[sp_index].image->height = 23;
	} else if (size == 4) {
		OAM[sp_index].image->x = 8;
		OAM[sp_index].image->crop_width = 31;
		OAM[sp_index].image->height = 23;
	}
	sp_index++;

	if (size > 2) {
		if (currentSelect == saves) {
			bool blink = counter%64 < 32;
			if (select_save_state > 0) drawSprite(0xE7, x - (blink?16:14), 4 + y+(size-1)*4, 0x19, 1, 2);
			if (select_save_state < 3) drawSprite(0xE8, x+(size+1)*8 + (blink?15:13),	4 + y+(size-1)*4, 0x19, 1, 2);
		} else {
			bool blink = counter%64 < 32;
			drawSprite(0xE3, x+4+(size>>1)*8, 	y-2+size*8, 0x07, 2, 1);
			drawSprite(blink?0xF3:0xF4, x+4+(size>>1)*8, 	y+6+size*8, 0x07|(blink?0:0x80), 1, 1);
			drawSprite(blink?0xF3:0xF4, x+12+(size>>1)*8, 	y+6+size*8, 0x07|(blink?0x80:0), 1, 1);
		}
	} else {
		drawSprite(0xE5, x + size*8 - 4, y-4, 0x07, 2, 0, opacity);
	}

	if (size < 4) {
		if (counter%30 < 15) {
			drawSprite(0xC2, x + 13 + 8 * size, y, 0x07, 2, 0, opacity);
			drawSprite(0xC2, x - 6, y, 0x07|0x80, 2, 0, opacity);
		} else {
			if (grad > 40 && grad < 210) {
				drawSprite(0xC4, x + 13 + 8 * size, y, 0x07, 2, 0, opacity);
				drawSprite(0xC4, x - 6, y, 0x07|0x80, 2, 0, opacity);
			} else {
				drawSprite(0xC2, x + 13 + 8 * size, y, 0x07, 2, 0, opacity);
				drawSprite(0xC2, x - 6, y, 0x07|0x80, 2, 0, opacity);
			}
		}
	} else {
		if (counter%30 < 15) {
			drawSprite(0xB0, x + 13 + 8 * size, y-8, 0x07, 2,3);
			drawSprite(0xB0, x - 6, y-8, 0x07|0x80, 2,3);
		} else {
			if (grad > 40 && grad < 210) {
				drawSprite(0xE0, x + 13 + 8 * size, y, 0x07, 3,2);
				drawSprite(0xE0, x - 6, y, 0x07|0x80, 3, 2);
			} else {
				drawSprite(0xB0, x + 13 + 8 * size, y-8, 0x07, 2, 3);
				drawSprite(0xB0, x - 6, y-8, 0x07|0x80, 2, 3);
			}
		}	
		std::string minutes = std::to_string(temp_save_state->time_shtamp / 60);
		std::string seconds = std::to_string(temp_save_state->time_shtamp % 60);
		if (seconds.length()==1) seconds = "0"+seconds;
		const char * stamp = (minutes+":"+seconds).c_str();
		uint8_t leng = seconds.length() + minutes.length() + 1;
		for (uint8_t i = 0; i < leng; i++)
		drawSprite(stamp[i]-47, x +(35-leng*4) + 4 * i, y+28, 0x3B, 1);
	}
}

void Shell::drawSavesPanel() {
	uint8_t offset = stable_position+4;
	uint8_t l = 9;
	memset(&tblName[17*TBL_WIDTH], 0x01, TBL_WIDTH);
	memset(&tblName[18*TBL_WIDTH], 0x20, TBL_WIDTH*12);
	memset(&tblAttribute[17*TBL_WIDTH], 0x01, TBL_WIDTH*13);
	for (uint8_t i = 0; i<4; i++) {
		tblName[((i>>1)+16)*TBL_WIDTH + offset+(i%2)] = 0x31 + (0x10*(i>>1));
		tblAttribute[((i>>1)+16)*TBL_WIDTH + offset+(i%2)] = 0x01 + (0x80*(!(i%2)));
	}
	for (uint8_t c = 0; c <= l; c++) {
		tblName[(c+19)*TBL_WIDTH + 2] = (c==0||c==l)?0x3E:0x4E;
		tblName[(c+19)*TBL_WIDTH + TBL_WIDTH-3] = (c==0||c==l)?0x3E:0x4E;
		memset(&tblName[(c+19)*TBL_WIDTH + 3],(c==0||c==l)?0x3F:0x4F, TBL_WIDTH-6);
		memset(&tblAttribute[(c+19)*TBL_WIDTH + 2], (c==0||c==l)?(c==l?0x4C:0x0C):(c==l-2?0x10:0x14), TBL_WIDTH-4);
		tblAttribute[(c+19)*TBL_WIDTH + TBL_WIDTH-3] |= 0x80;
	}
	drawSaveState(1, 4, 20);
	drawSaveState(2, 12, 20);
	drawSaveState(3, 20, 20);

	if (SCREEN_SIZE>256) drawSaveState(4, 28, 20);
	offset = HALF_TBL_WIDTH - 10;
	drawSprite(0x46, offset<<3, 17<<3, 0x03, 2);
	memcpy(&tblName[18*TBL_WIDTH + offset+2], "Suspend Point List", 18);
	memset(&tblAttribute[18*TBL_WIDTH + offset+2], 0x23, 18);
}

void Shell::drawSaveState(uint8_t ndx, uint8_t xt, uint8_t yt) {
	uint8_t pallete_1 = 0x1C;
	uint8_t pallete_2 = 0x1D;

	if (!temp_save_state && stable_position_syspend_panel_selector==6 + 8*(ndx-1)) {
		pallete_1 = 0x1E;
		pallete_2 = 0x1F;
	}

	uint16_t tbl_sv_st_16[4][2] = {
		{0x4342, 0x5352},
		{0x4544, 0x5554},
		{0x9190, 0xA1A0},
		{0x9392, 0xA3A2},
	};
	memcpy(&tblName[yt*TBL_WIDTH + xt], &tbl_sv_st_16[ndx-1][0], 2);
	memcpy(&tblName[(yt+1)*TBL_WIDTH + xt], &tbl_sv_st_16[ndx-1][1], 2);
	memset(&tblAttribute[yt*TBL_WIDTH + xt], 0x0F, 2);
	memset(&tblAttribute[(yt+1)*TBL_WIDTH + xt], 0x0F, 2);
	for(uint8_t i=1; i<4; i++) {
		memset(&tblName     [(yt+i)*TBL_WIDTH + xt + 3], 0x20, 4);
		memset(&tblAttribute[(yt+i)*TBL_WIDTH + xt + 3], (courusel[sel]->savePointList[ndx-1]?pallete_1:0x0A), 4);
	}
	memset(&tblAttribute[(yt+4)*TBL_WIDTH + xt + 3], (courusel[sel]->savePointList[ndx-1]?pallete_2:0x12), 4);
	memset(&tblAttribute[ yt   *TBL_WIDTH + xt + 4], (courusel[sel]->savePointList[ndx-1]?pallete_2:0x13), 2);
	memset(&tblName		[ yt   *TBL_WIDTH + xt + 3], 0x5E, 4);
	memcpy(&tblName[(yt+2)*TBL_WIDTH + xt+4], "NO", 2);
	memset(&tblAttribute[(yt+2)*TBL_WIDTH + xt+4], 0x32, 2);
	tblName[ yt   *TBL_WIDTH + xt+2] = 
	tblName[ yt   *TBL_WIDTH + xt+7] = 0x5F;
	tblName[(yt+1)*TBL_WIDTH + xt+2] = 
	tblName[(yt+1)*TBL_WIDTH + xt+7] = 
	tblName[(yt+2)*TBL_WIDTH + xt+2] = 
	tblName[(yt+2)*TBL_WIDTH + xt+7] = 
	tblName[(yt+3)*TBL_WIDTH + xt+2] = 
	tblName[(yt+3)*TBL_WIDTH + xt+7] = 0x6F;
	tblName[(yt+4)*TBL_WIDTH + xt+2] = 0x5D;
	tblName[(yt+4)*TBL_WIDTH + xt+3] = 0x9A;
	tblName[(yt+4)*TBL_WIDTH + xt+4] =
	tblName[(yt+4)*TBL_WIDTH + xt+5] = 0x9B;
	tblName[(yt+4)*TBL_WIDTH + xt+6] = 0x9C;
	tblName[(yt+4)*TBL_WIDTH + xt+7] = 0x7F;
	tblAttribute[ yt   *TBL_WIDTH + xt+3] = (courusel[sel]->savePointList[ndx-1]?pallete_2:0x0E) | 0x80;	
	tblAttribute[ yt   *TBL_WIDTH + xt+6] = (courusel[sel]->savePointList[ndx-1]?pallete_1:0x12);
	tblAttribute[ yt   *TBL_WIDTH + xt+2] = 
	tblAttribute[(yt+1)*TBL_WIDTH + xt+2] = 
	tblAttribute[(yt+2)*TBL_WIDTH + xt+2] = 
	tblAttribute[(yt+3)*TBL_WIDTH + xt+2] = (courusel[sel]->savePointList[ndx-1]?pallete_1:0x06) | 0x80;
	tblAttribute[(yt+4)*TBL_WIDTH + xt+2] =
	tblAttribute[ yt   *TBL_WIDTH + xt+7] = 
	tblAttribute[(yt+1)*TBL_WIDTH + xt+7] = 	
	tblAttribute[(yt+2)*TBL_WIDTH + xt+7] = 
	tblAttribute[(yt+3)*TBL_WIDTH + xt+7] = 
	tblAttribute[(yt+4)*TBL_WIDTH + xt+7] = (courusel[sel]->savePointList[ndx-1]?pallete_1:0x16);

	if (courusel[sel]->savePointList[ndx-1]) {
		int16_t x = (xt+2) * 8;
		int16_t y = yt * 8;
		OAM[sp_index].x = (xt+2) * 8;
		OAM[sp_index].y = yt * 8;
		OAM[sp_index].opacity = 100;
		OAM[sp_index].image = courusel[sel]->savePointList[ndx-1]->image;
		OAM[sp_index].image->crop_width = 32;
		OAM[sp_index].image->height = 23;
		sp_index++;
		std::string minutes = std::to_string(courusel[sel]->savePointList[ndx-1]->time_shtamp / 60);
		std::string seconds = std::to_string(courusel[sel]->savePointList[ndx-1]->time_shtamp % 60);
		if (seconds.length()==1) seconds = "0"+seconds;
		const char * stamp = (minutes+":"+seconds).c_str();
		uint8_t leng = seconds.length() + minutes.length() + 1;
		for (uint8_t i = 0; i < leng; i++)
		drawSprite(stamp[i]-47, x +(35-leng*4) + 4 * i, y+29, 0x3B, 1);
	}
}

void Shell::drawSettingsDisplayContent(uint8_t counter) {

	for (uint8_t i = 0; i<5; i++)
	memset(&tblAttribute[(i+8+6*displaySettingSelector)*TBL_WIDTH + 8], 0x1A, TBL_WIDTH-16);

	tblName[10*TBL_WIDTH + HALF_TBL_WIDTH - 7] = disaplayScale == 0?0xA8:0xA7;
	memcpy(&tblName[10*TBL_WIDTH + HALF_TBL_WIDTH - 5], "CRT filter", 10);
	memset(&tblAttribute[10*TBL_WIDTH + HALF_TBL_WIDTH - 5], displaySettingSelector==0?0x3A:0x27, 10);

	tblName[16*TBL_WIDTH + HALF_TBL_WIDTH - 7] = disaplayScale == 1?0xA8:0xA7;
	memcpy(&tblName[16*TBL_WIDTH + HALF_TBL_WIDTH - 5], "4:3", 3);
	memset(&tblAttribute[16*TBL_WIDTH + HALF_TBL_WIDTH - 5], displaySettingSelector==1?0x3A:0x27, 3);

	tblName[22*TBL_WIDTH + HALF_TBL_WIDTH - 7] = disaplayScale == 2?0xA8:0xA7;
	memcpy(&tblName[22*TBL_WIDTH + HALF_TBL_WIDTH - 5], "Pixel Perfect", 13);
	memset(&tblAttribute[22*TBL_WIDTH + HALF_TBL_WIDTH - 5], displaySettingSelector==2?0x3A:0x27, 13);

	if (!counter) cursor = (++cursor)%3;
	for (uint8_t i = 0; i<2; i++) {
		tblName[(7+6*displaySettingSelector)*TBL_WIDTH + HALF_TBL_WIDTH + i - 1] =  0x0A+cursor;
		tblAttribute[(7+6*displaySettingSelector)*TBL_WIDTH + HALF_TBL_WIDTH + i - 1]  = !i?0x9B:0x1B;
	}
}

void Shell::drawSettingsDisplayPanel(uint8_t counter) {	
	for (uint8_t i = 0; i < 30; i++) {
		switch (i) {
			case 0: 	tblName[i * TBL_WIDTH + 0] = 0x94;
						memset(&tblName[i * TBL_WIDTH + 1], 0x95, TBL_WIDTH-2);
						tblName[i * TBL_WIDTH + (TBL_WIDTH-1)] = 0x94;
						tblAttribute[i * TBL_WIDTH + (TBL_WIDTH-1)] |= 0x80;
			break;
			case 4:		tblName[i * TBL_WIDTH] = 0xA5;
						memset(&tblName[i * TBL_WIDTH + 1], 0x95, TBL_WIDTH - 2);
						tblName[i * TBL_WIDTH + (TBL_WIDTH-1)] = 0xA5;
						tblAttribute[i * TBL_WIDTH + (TBL_WIDTH-1)] |= 0x80;
			break;
			case 29:	tblName[i * TBL_WIDTH + 0] = 0x94;
						tblAttribute[i * TBL_WIDTH + 0] |= 0x40;
						memset(&tblName[i * TBL_WIDTH + 1], 0x95, TBL_WIDTH-2);
						tblName[i * TBL_WIDTH + (TBL_WIDTH-1)] = 0x94;
						tblAttribute[i * TBL_WIDTH + (TBL_WIDTH-1)] |= 0x80 | 0x40;
			break;
			default:	tblName[i * TBL_WIDTH] = 0xA4;
						tblName[i * TBL_WIDTH + (TBL_WIDTH - 1)] = 0xA4;
			break;
		}
	}
	drawSprite(0x60, 11, 8, 0x07, 3);
	memcpy(&tblName[(TBL_WIDTH<<1) + 5], "Display", 7);
	memset(&tblAttribute[(TBL_WIDTH<<1) + 5], 0x27, 7);
	drawSettingsDisplayContent(counter);
}

void Shell::drawTitleBar(uint8_t x, uint8_t y, uint8_t l) {
	l_y = y;
	for (uint8_t i = 0; i<l; i++) {
		tblName[y*TBL_WIDTH + i + x] = tblName[(y+2)*TBL_WIDTH + i + x] = i==0?0x3A:(i==l-1?0x3A:0x39);
		tblName[(y+1)*TBL_WIDTH + i + x] = i==0?0x59:(i==l-1?0x59:0x4B);
		tblAttribute[y*TBL_WIDTH + i + x] = (i==0?0x80:0x00) | 0x11;
		tblAttribute[(y+1)*TBL_WIDTH + i + x] = (i!=0?0x80:0x00) | 0x11;	
		tblAttribute[(y+2)*TBL_WIDTH + i + x] = (i!=0?0x40:0xC0) | 0x11;
	}
}

void Shell::drawHintBar(selectors mode) {
	uint8_t offset = 0;
	uint8_t select [4] = {0x02,0x03,0x04,0x05};
	uint8_t start [3] = {0x06,0x07,0x08};
	uint8_t add = SCREEN_SIZE>256?1:0;
	switch (mode) {
		case menu:
			offset = (TBL_WIDTH-24) >> 1;
			tblName[25*TBL_WIDTH+offset] = 0x0F;
			tblAttribute[25*TBL_WIDTH+offset] = 0x02;
			offset+=1;
			memcpy(&tblName[25*TBL_WIDTH+offset], "Games", 5);
			memset(&tblAttribute[25*TBL_WIDTH+offset], 0x22, 5);
			offset+=6;
			tblName[25*TBL_WIDTH+offset] = 0x7C;
			tblAttribute[25*TBL_WIDTH+offset] = 0x02;
			offset+=1;
			memcpy(&tblName[25*TBL_WIDTH+offset], "Select", 6);
			memset(&tblAttribute[25*TBL_WIDTH+offset], 0x22, 6);
			offset+=7;
			tblName[25*TBL_WIDTH+offset] = 0x51;
			tblAttribute[25*TBL_WIDTH+offset] = 0x02;
			offset+=1;
			memcpy(&tblName[25*TBL_WIDTH+offset], "Back", 4);
			memset(&tblAttribute[25*TBL_WIDTH+offset], 0x22, 4);
			offset+=5;
			tblName[25*TBL_WIDTH+offset] = 0x50;
			tblAttribute[25*TBL_WIDTH+offset] = 0x02;
			offset+=1;
			memcpy(&tblName[25*TBL_WIDTH+offset], "OK", 2);
			memset(&tblAttribute[25*TBL_WIDTH+offset], 0x22, 2);
		break;
		case empty:
		case gamelist:
			offset = (TBL_WIDTH-(28+add)) >> 1;
			tblName[25*TBL_WIDTH+offset] = 0x0E;
			tblAttribute[25*TBL_WIDTH+offset] = 0x02;
			offset+=1;
			memcpy(&tblName[25*TBL_WIDTH+offset], "Menu", 4);
			memset(&tblAttribute[25*TBL_WIDTH+offset], 0x22, 4);
			offset+=5;
			tblName[25*TBL_WIDTH+offset] = 0x0F;
			tblAttribute[25*TBL_WIDTH+offset] = 0x02;
			offset+=1;
			memcpy(&tblName[25*TBL_WIDTH+offset], "Saves", 5);
			memset(&tblAttribute[25*TBL_WIDTH+offset], 0x22, 5);
			offset+=5+add;
			memcpy(&tblName[25*TBL_WIDTH+offset], &select, 4);
			memset(&tblAttribute[25*TBL_WIDTH+offset], 0x02, 4);
			offset+=4;
			memcpy(&tblName[25*TBL_WIDTH+offset], "Sort", 4);
			memset(&tblAttribute[25*TBL_WIDTH+offset], 0x22, 4);
			offset+=5;
			memcpy(&tblName[25*TBL_WIDTH+offset], &start, 3);
			memset(&tblAttribute[25*TBL_WIDTH+offset], 0x02, 4);
			offset+=3;
			memcpy(&tblName[25*TBL_WIDTH+offset], "Game", 4);
			memset(&tblAttribute[25*TBL_WIDTH+offset], 0x22, 4);
		break;
		case saves:
			if (temp_save_state && courusel[sel]->id == temp_save_state->id) {
				offset = (TBL_WIDTH-(24+add+add+add)) >> 1;
				tblName[27*TBL_WIDTH+offset] = 0x7C;
				tblAttribute[27*TBL_WIDTH+offset] = 0x0F;
				offset+=1;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Select", 6);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 6);
				offset+=6+add;
				memcpy(&tblName[27*TBL_WIDTH+offset], &start, 3);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x0F, 3);
				offset+=3;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Game", 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 4);
				offset+=4+add;
				tblName[27*TBL_WIDTH+offset] = 0x51;
				tblAttribute[27*TBL_WIDTH+offset] = 0x0F;
				offset+=1;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Back", 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 4);
				offset+=4+add;
				tblName[27*TBL_WIDTH+offset] = 0x50;
				tblAttribute[27*TBL_WIDTH+offset] = 0x0F;
				offset+=1;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Save", 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 4);
			} else {
				offset = (TBL_WIDTH-(27+add+add)) >> 1;
				tblName[27*TBL_WIDTH+offset] = 0x7C;
				tblAttribute[27*TBL_WIDTH+offset] = 0x0F;
				offset+=1;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Select", 6);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 6);
				offset+=6+add;
				memcpy(&tblName[27*TBL_WIDTH+offset], &select, 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x0F, 4);
				offset+=4;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Move", 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 4);
				offset+=5;
				memcpy(&tblName[27*TBL_WIDTH+offset], &start, 3);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x0F, 4);
				offset+=3;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Game", 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 4);
				offset+=4+add;
				tblName[27*TBL_WIDTH+offset] = 0x51;
				tblAttribute[27*TBL_WIDTH+offset] = 0x0F;
				offset+=1;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Back", 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 4);
			}
		break;
		case displaySettings:
				offset = TBL_WIDTH - 1 - 18;
				tblName[2*TBL_WIDTH+offset] = 0xA6;
				offset+=1;
				memcpy(&tblName[2*TBL_WIDTH+offset], "Select", 6);
				memset(&tblAttribute[2*TBL_WIDTH+offset], 0x27, 6);
				offset+=7;
				tblName[2*TBL_WIDTH+offset] = 0x98;
				offset+=1;
				memcpy(&tblName[2*TBL_WIDTH+offset], "Back", 4);
				memset(&tblAttribute[2*TBL_WIDTH+offset], 0x27, 4);
				offset+=5;
				tblName[2*TBL_WIDTH+offset] = 0x97;
				offset+=1;
				memcpy(&tblName[2*TBL_WIDTH+offset], "OK", 2);
				memset(&tblAttribute[2*TBL_WIDTH+offset], 0x27, 2);
		break;
	}
}

void Shell::drawSelector(selectors mode) {
	if (select_to > stable_position) stable_position++;
	else if (select_to < stable_position) stable_position--;

	if (select_to_menu_position > select_menu_position) select_menu_position++;
	else if (select_to_menu_position < select_menu_position) select_menu_position--;

	if (select_to_syspend_panel_selector > stable_position_syspend_panel_selector) stable_position_syspend_panel_selector++;
	else if (select_to_syspend_panel_selector < stable_position_syspend_panel_selector) stable_position_syspend_panel_selector--;

	uint8_t y = 0;
	uint8_t h = 0;
	uint8_t w = 0;
	uint16_t start_menu_select_position_offset =  (SCREEN_SIZE>>1)-64;
	switch (mode) {
		case menu: y = 1; h = 3; w = 4;
			for (uint8_t ys = 0; ys <= h; ys++)
				for (uint8_t xs = 0; xs <= w; xs++) {
					if (ys==0||ys==h) drawSprite((xs==0||xs==w)?0x6E:0x58, (xs<<3) + start_menu_select_position_offset + (select_menu_position<<3)-4, ((y+ys)<<3)-2-(ys==h?3:0), 0x15 | ((xs==w)?0x80:0x00) | (ys==h?0x40:0x00));
					else if (xs==0||xs==w) drawSprite(0x7E, (xs<<3) + start_menu_select_position_offset + (select_menu_position<<3)-4, ((y+ys)<<3)-2, 0x15 | ((xs==w)?0x80:0x00));
					if (select_to_menu_position == select_menu_position)
					if (ys>0 && xs<w)
					tblAttribute[ (ys * TBL_WIDTH) + HALF_TBL_WIDTH - 8 + xs + select_menu_position] = 0x02;
				}
		break;
		case empty: break;
		case preparegame:
		case gamelist: y = 9; h = 10; w = 10;
				drawSprite(0x4A, (stable_position<<3)-2, 		(y<<3)-2,		0x95);
				drawSprite(0x4A, (stable_position<<3)-2,		((y+h)<<3)+1,	0xD5);
				drawSprite(0x4A, ((stable_position+w)<<3) - 7,	(y<<3)-2,		0x15);
				drawSprite(0x4A, ((stable_position+w)<<3) - 7,	((y+h)<<3)+1,	0x55);
				for (uint8_t c = 1; c < w; c++) {
					drawSprite(0x49, ((stable_position+c)<<3) - 4,	(y<<3)-2,		0x15);
					drawSprite(0x49, ((stable_position+c)<<3) - 4,	((y+h)<<3)+1,	0x55);
				}
				for (uint8_t c = 1; c <= h; c++) {
					drawSprite(0x48, (stable_position<<3)-2,		((y+c)<<3)-4,	0x15);
					drawSprite(0x48, ((stable_position+w)<<3)-7,	((y+c)<<3)-4,	0x95);
				}
		break;
		case saves: 
			if (temp_save_state && courusel[sel]->id == temp_save_state->id) {
				h = 4; w = 6;
				drawSprite(0x4A, temp_save_state_x - 2, temp_save_state_y - 2,	0x95);
				drawSprite(0x4A, temp_save_state_x - 2, temp_save_state_y + (h<<3) + 1,	0xD5);
				drawSprite(0x4A, temp_save_state_x + (w<<3) - 7,	temp_save_state_y-2, 0x15);
				drawSprite(0x4A, temp_save_state_x + (w<<3) - 7,	temp_save_state_y + (h<<3) + 1,	0x55);
				for (uint8_t c = 1; c < w; c++) {
					drawSprite(0x49, temp_save_state_x + (c<<3) - 4,	temp_save_state_y - 2,		0x15);
					drawSprite(0x49, temp_save_state_x + (c<<3) - 4,	temp_save_state_y + (h<<3) + 1,	0x55);
				}
				for (uint8_t c = 1; c <= h; c++) {
					drawSprite(0x48, temp_save_state_x - 2,		temp_save_state_y + (c<<3) - 4,	0x15);
					drawSprite(0x48, temp_save_state_x + (w<<3) - 7, temp_save_state_y + (c<<3) - 4,	0x95);
				}
			} else {
				if (no_select_state) break;
				y = 20; h = 4; w = 6;
				drawSprite(0x4A, (stable_position_syspend_panel_selector<<3)-1, 		(y<<3)-3,		0x95);
				drawSprite(0x4A, (stable_position_syspend_panel_selector<<3)-1,		((y+h)<<3)+3,	0xD5);
				drawSprite(0x4A, ((stable_position_syspend_panel_selector+w)<<3) - 7,	(y<<3)-3,		0x15);
				drawSprite(0x4A, ((stable_position_syspend_panel_selector+w)<<3) - 7,	((y+h)<<3)+3,	0x55);
				for (uint8_t c = 1; c < w; c++) {
					drawSprite(0x49, ((stable_position_syspend_panel_selector+c)<<3) - 4,	(y<<3)-3,		0x15);
					drawSprite(0x49, ((stable_position_syspend_panel_selector+c)<<3) - 4,	((y+h)<<3)+3,	0x55);
				}
				for (uint8_t c = 1; c <= h; c++) {
					drawSprite(0x48, (stable_position_syspend_panel_selector<<3)-1,		((y+c)<<3)-3,	0x15);
					drawSprite(0x48, ((stable_position_syspend_panel_selector+w)<<3)-7,	((y+c)<<3)-3,	0x95);
				}
			}
		break;
	}
}

uint32_t Shell::avrColor(uint8_t a, uint8_t b, uint32_t c1, uint32_t c2) {
	uint8_t R = ((((c1 >> 16) & 0xFF) * a) + (((c2 >> 16) & 0xFF)*b)) / (a+b);
	uint8_t G = ((((c1 >> 8) & 0xFF) * a) + (((c2 >> 8) & 0xFF)*b)) / (a+b);
	uint8_t B = (((c1 & 0xFF) * a) + ((c2 & 0xFF)*b)) / (a+b);
	return (R << 16) | (G<<8) | B;
}

void Shell::PutPixel(int16_t px,int16_t py, uint32_t color) {
	if (currentSelect == preparegame) color = avrColor(100-fadeProcents, fadeProcents, color, 0);
	else if (fadeProcents>0) color = avrColor(100-fadeProcents, fadeProcents, color, 0);
	FrameBuffer[(py*SCREEN_SIZE) + px] = color;
}

uint32_t Shell::GetPixel(int16_t px,int16_t py) {
	return FrameBuffer[(py*SCREEN_SIZE) + px];
}

void Shell::PlayWav(WavFile * file) {
	if (file->file) fseek(file->file, file->offset, SEEK_SET);
}

void Shell::PlayWavClock(WavFile * file, bool loop) {
	if (file && file->file)
	if (!feof(file->file)) {
		MixArray(audiosample, tempsample, fread(tempsample, sizeof(int16_t), audioBufferCount, file->file));
		if (loop && feof(file->file)) fseek(file->file, file->offset, SEEK_SET);
	}
}

void Shell::Update(double FPS) {

#ifdef DEBUG
printf("DEBUG: %s.\n", "Clear audio buffer");
#endif

	memset(audiosample, 0x00, 2048<<1);
	memset(tempsample, 0x00, 2048<<1);
	audioBufferCount = SoundSamplesPerSec/FPS + 0.5f;

#ifdef DEBUG
printf("DEBUG: %s.\n", "Playing always sound clock");
#endif

	PlayWavClock(se_sys_click_game);
	PlayWavClock(se_sys_smoke);

	if (currentSelect == playgame){
		if (CART) {
			NES.CPU.controller[0] = controller;
			uint8_t SkeepBuffer = (int)(1789773.0/(SoundSamplesPerSec>>1)) * ((double)FPS/59.5f);
			uint16_t totalCount = audioBufferCount;
			audioBufferCount = 0;		
			do {
				NES.Clock();
				clock_counter++;
				if (totalCount>0 && NES.APU.getAudioReady(SkeepBuffer)) {
					tempsample[audioBufferCount++] =  NES.APU.sample;
					totalCount--;
				}
			} while (!NES.PPU.frame_complete);
			// если не хватило семплов до полного буффера одного кадра, то добиваем. 
			while (totalCount>0) {
				NES.APU.clock();
				if (NES.APU.getAudioReady(SkeepBuffer)) {
					tempsample[audioBufferCount++] =  NES.APU.sample;
					totalCount--;
				}
			}
			NES.PPU.frame_complete = false;
			if (clock_counter < 476490) memset(FrameBuffer, 0x00, SCREEN_SIZE * 240 << 2); //Костыль, чтобы не моргал экран при загрузке
			MixArray(audiosample, tempsample, audioBufferCount);
		}
		if (controller&0x10 && controller&0x20) {
			temp_save_state = new SavePoint();
			temp_save_state->time_shtamp = clock_counter / 1789773;
			temp_save_state->id = courusel[sel]->id;
			temp_save_state->image = new Image();
			temp_save_state->image->width = 32;
			temp_save_state->image->height = 24;
			temp_save_state->image->x = 8;
			temp_save_state->image->y = 4;
			uint8_t scale = SCREEN_SIZE / temp_save_state->image->width;
			for (uint8_t y = 0; y< temp_save_state->image->height; y++)
			for (uint8_t x = 0; x< temp_save_state->image->width; x++) { 
				BmpPixel bmpPixel;
				uint32_t pixel1 = FrameBuffer[(y*SCREEN_SIZE + x)*scale];
				bmpPixel.R = (pixel1 >> 16) & 0xFF;
				bmpPixel.G = (pixel1 >> 8) & 0xFF;
				bmpPixel.B = (pixel1 >> 0) & 0xFF;	
				for (uint8_t y1 = 1; y1< scale; y1++) 
				for (uint8_t x1 = 1; x1< scale; x1++) {
					uint32_t pixel2 = FrameBuffer[(y*scale+y1)*SCREEN_SIZE + (x*scale+x1)];
					bmpPixel.R = (((pixel2 >> 16) & 0xFF)+bmpPixel.R)>>1;
					bmpPixel.G = (((pixel2 >> 8) & 0xFF)+bmpPixel.G)>>1;
					bmpPixel.B = (((pixel2 >> 0) & 0xFF)+bmpPixel.B)>>1;	
				}
				temp_save_state->image->rawData.push_back(bmpPixel);
			}
			x_offset_save_state = x_offset;
			stable_position_save_state = stable_position;
			NES.RejectCartridge();
			delete CART;
			CART = nullptr;
			currentSelect = gamelist; // exit;
		}
		return;
	}
	if (currentSelect == preparegame && fadeProcents == 100 && !temp_save_state) {
		memset(FrameBuffer, 0x00, SCREEN_SIZE * 240 << 2);
		currentSelect = playgame;
		clock_counter = 0;
		return;
	} 
	counter++;

/* play music */
#ifdef DEBUG
printf("DEBUG: %s.\n", "Playing sound clock");
#endif

	if (bgm_boot) {
		PlayWavClock(bgm_boot);
		if (feof(bgm_boot->file)) {
			fclose(bgm_boot->file);
			bgm_boot->file = NULL;
			delete bgm_boot;
			bgm_boot = NULL;
			
		}
	} else PlayWavClock(bgm_home, true);
	PlayWavClock(se_sys_cancel);
	PlayWavClock(se_sys_click);
	PlayWavClock(se_sys_cursor);

#ifdef DEBUG
printf("DEBUG: %s.\n", "Clear OAM");
#endif
	sp_index = 0;
	memset(OAM, 0x00, MAX_SPRITES * sizeof(sObjectAttributeEntry));
	/* update */
#ifdef DEBUG
printf("DEBUG: %s.\n", "Update controller action");
#endif
		if (controller&0x01) {
			switch (currentSelect) {
				case menu:
					if (!(lastkbState&0x01) || counter%10==0 )
					if (select_menu_position == select_to_menu_position) {
						select_to_menu_position += 4;
						if (select_to_menu_position > 12) select_to_menu_position = 12;
						PlayWav(se_sys_cursor);
					}
				break;
				case gamelist:
					if (courusel_direction == 0 && stable_position == select_to) {
						select_to+=11;
						if (select_to > TBL_WIDTH - 14) {
							select_to = TBL_WIDTH - 14;
							courusel_direction=-1;
						}
						PlayWav(se_sys_cursor);
					}
				break;
				case saves:
					if (!(lastkbState&0x01) || counter%20==0) {
						if (temp_save_state && courusel[sel]->id == temp_save_state->id) {
							if (select_save_state < 3) {
								select_save_state++;
								PlayWav(se_sys_cursor);
							}
						} else {
							if (select_save_state+1 < 4)
							for (uint8_t i = select_save_state+1; i < 4; i++) {
								if (courusel[sel]->savePointList[i]) {
									select_to_syspend_panel_selector = 6 + 8*i;
									select_save_state = i;
									PlayWav(se_sys_cursor);
									break;
								}
							}
						}
					}
				break;
			}
		}
		if (controller&0x02) {
			switch (currentSelect) {
				case menu:
					if (!(lastkbState&0x02) || counter%10==0 )
					if (select_menu_position == select_to_menu_position) {
						select_to_menu_position -= 4;
						if (select_to_menu_position < 0) select_to_menu_position = 0;
						PlayWav(se_sys_cursor);
					}
				break;

				case gamelist:
					if (courusel_direction == 0 && stable_position == select_to) {
						select_to-=11;
						if (select_to < 4 ) {
							select_to = 4 ;
							courusel_direction=1;
						}
						PlayWav(se_sys_cursor);
					}
				break;
				case saves:
					if (!(lastkbState&0x02) || counter%20==0) {
						if (temp_save_state && courusel[sel]->id == temp_save_state->id) {
							if (select_save_state > 0) {
								select_save_state--;
								PlayWav(se_sys_cursor);
							}
						} else {
							if (select_save_state > 0)
							for (int8_t i = select_save_state-1; i >= 0; i--) {
								if (courusel[sel]->savePointList[i]) {
									select_to_syspend_panel_selector = 6 + 8*i;
									select_save_state = i;
									PlayWav(se_sys_cursor);
									break;
								}
							}
						}
					}
				break;
			}
		}

		if (controller&0x20&& !(lastkbState&0x20)) {
			switch (currentSelect) {
				case saves:
					if (!temp_save_state) {
						if (courusel[sel]->savePointList[select_save_state]) {
							x_offset_save_state = x_offset;
							stable_position_save_state = stable_position;
							temp_save_state = courusel[sel]->savePointList[select_save_state];
							courusel[sel]->savePointList[select_save_state] = nullptr;
							PlayWav(se_sys_cursor);
						}
					}
				break;
			}
		}


		if (controller&0x10&& !(lastkbState&0x10)) {
			switch (currentSelect) {
				case gamelist:
					PlayWav(se_sys_click_game);
					if (CART) {NES.RejectCartridge(); delete CART; CART = nullptr;}
					CART = new CARTRIDGE(courusel[sel]->exec.c_str());
					if (!CART->ImageValid()) {
						delete CART;
						CART = nullptr;
						return;
					};
					NES.ConnectCartridge(CART);
					memset(FrameBuffer, 0x00, SCREEN_SIZE * 240 << 2);
					NES.PPU.setFrameBuffer(FrameBuffer);
					NES.PPU.setScale(SCREEN_SIZE, TBL_WIDTH<40?0:disaplayScale);
					NES.Reset();
					if (temp_save_state) {
						select_save_state = -1;
						PlayWav(se_sys_smoke);
					}
					currentSelect = preparegame;
				break;
			}
		}
		if (controller&0x40&& !(lastkbState&0x40)) {
			if (currentSelect == gamelist)
				if (temp_save_state && courusel[sel]->id != temp_save_state->id) {
					for (uint8_t i = 0; i < courusel.size(); i++) {
						if (courusel[i]->id == temp_save_state->id) {
							x_offset = x_offset_save_state;
							stable_position = stable_position_save_state;
							select_to = stable_position;
							PlayWav(se_sys_cursor);
							break;
						}
					}
				}
			switch (currentSelect) {
				case menu:	
				case saves: currentSelect = gamelist; PlayWav(se_sys_cancel); break;
				case displaySettings: currentSelect = menu;	PlayWav(se_sys_cancel);	break;
			}
		}
		if (controller&0x80 && !(lastkbState&0x80)) {
			switch (currentSelect) {
				case menu:
					if (select_to_menu_position == 0) currentSelect = displaySettings;
					PlayWav(se_sys_click);
				break;
				case displaySettings:
					disaplayScale = displaySettingSelector;
					PlayWav(se_sys_click);
				break;
				case saves:
					if (temp_save_state && courusel[sel]->id == temp_save_state->id) {
						if (courusel[sel]->savePointList[select_save_state]) {
							delete courusel[sel]->savePointList[select_save_state]->image;
							courusel[sel]->savePointList[select_save_state]->image = nullptr;
							delete courusel[sel]->savePointList[select_save_state];
							courusel[sel]->savePointList[select_save_state] = nullptr;
						}
						courusel[sel]->savePointList[select_save_state] = temp_save_state;
						stable_position_syspend_panel_selector = 6 + 8 * select_save_state;
						select_to_syspend_panel_selector = stable_position_syspend_panel_selector;
						temp_save_state = nullptr;
						no_select_state = false;
						PlayWav(se_sys_click);
					}
				break;
			}
		}
		if (controller&0x08 && !(lastkbState&0x08)) {
			if ((currentSelect == gamelist)||(currentSelect == empty)) currentSelect = menu;
			else if (currentSelect == saves) currentSelect = gamelist;
			else if (currentSelect == displaySettings) displaySettingSelector = --displaySettingSelector<0?2:displaySettingSelector;
			if (currentSelect != playgame) PlayWav(se_sys_cursor);
		}
		if (controller&0x04 && !(lastkbState&0x04)) {
			if (currentSelect == menu) currentSelect = currentSelect = courusel.size()==0?empty:gamelist;
			else if (currentSelect == gamelist) {
				if (courusel[sel]) {
					currentSelect = saves;
					if (!courusel[sel]->savePointList[select_save_state]){
						no_select_state = true;
						for (uint8_t i = 0; i < 4; i++) {
							if (courusel[sel]->savePointList[i]) {
								select_to_syspend_panel_selector = 6 + 8*i;
								select_save_state = i;
								no_select_state = false;
								break;
							}
						}
					} else no_select_state = false;
				}
				printf("test\n");
			}
			else if (currentSelect == displaySettings) displaySettingSelector = ++displaySettingSelector%3;
			if (currentSelect != playgame) PlayWav(se_sys_cursor);
		}
	/**********/
#ifdef DEBUG 
printf("DEBUG: %s.\n", "Clear tblName");
#endif

	memset(&tblName[0], 0x20, TBL_SIZE);

#ifdef DEBUG 
printf("DEBUG: %s.\n", "Draw UI");
#endif

	if (currentSelect == displaySettings) {
		memset(&tblAttribute[0], 0x07, TBL_SIZE);
		drawSettingsDisplayPanel(counter%10);
		drawHintBar(currentSelect);
	} else {
		memset(&tblAttribute[0], 0x17, TBL_SIZE);
		drawTopBar();
		drawMenu();
		if (currentSelect == saves) {
			drawTitleBar(4, 4, TBL_WIDTH-8);
			drawCourusel(7, counter%1);
			drawSavesPanel();
			if (temp_save_state && courusel[sel]->id == temp_save_state->id) drawTempSaveState(counter);
		} else {
			drawDownBar();
			drawTitleBar(4, 6, TBL_WIDTH-8);
			drawCourusel(9, counter%1);
			drawNavigate(counter%10);
			drawTempSaveState(counter);
		}
		drawSelector(currentSelect);
		drawHintBar(currentSelect);
	}
	renderFrameBuffer();
	if (currentSelect == preparegame) {
		if (fadeProcents<100) fadeProcents+=10;
		if (fadeProcents>100) fadeProcents=100;
	} else {
		if (fadeProcents>0) fadeProcents-=10;
		if (fadeProcents<0) fadeProcents=0;
	}
}

void Shell::setJoyState(uint8_t kb) {
    #ifdef DEBUG
    printf("DEBUG: %s.\n", "Set Joy State");
    #endif
	lastkbState = controller;
    controller = kb;

//    controller |= kb[27/* X */] ? 0x80 : 0x00;     // A Button
//    controller |= kb[29/* Z */] ? 0x40 : 0x00;     // B Button
//    controller |= kb[229 /* RSHIFT */] ? 0x20 : 0x00;     // Select
//    controller |= kb[40 /* RETURN */] ? 0x10 : 0x00;     // Start
 //   controller |= kb[82 /* UP */] ? 0x08 : 0x00;
 //   controller |= kb[81 /* DOWN */] ? 0x04 : 0x00;
 //   controller |= kb[80 /* LEFT */] ? 0x02 : 0x00;
 //   controller |= kb[79 /* RIGHT*/] ? 0x01 : 0x00;
}

void Shell::renderFrameBuffer() {
#ifdef DEBUG 
printf("DEBUG: %s.\n", "Render screen");
#endif
	for (uint16_t y = 0; y < 240; y++)
	for (uint16_t x = 0; x < SCREEN_SIZE; x++) {
		uint16_t bkg_pattern = 0;
		uint8_t  bkg_bit_mux = 0;
		uint16_t bn = (y>>3) * TBL_WIDTH + (x>>3);
		if (tblAttribute[bn]&0x40) bkg_pattern = sprites[(tblAttribute[bn]&0x20)>>5].sprite[tblName[bn]].pattern[7-(y%8)];
							  else bkg_pattern = sprites[(tblAttribute[bn]&0x20)>>5].sprite[tblName[bn]].pattern[y%8];
		if (tblAttribute[bn]&0x80) {
										bkg_pattern >>= (x%8)<<1;
										bkg_bit_mux = bkg_pattern & 0x03;
									}  else {
										bkg_pattern <<= (x%8)<<1;
										bkg_bit_mux = (bkg_pattern & 0xC000) >> 14;
									}
		PutPixel(x, y, pallete[tblAttribute[bn]&0x1F].color[bkg_bit_mux]);
	}

	uint16_t max_length_pix = max_length << 3;
	uint32_t txt_color = 0xFF000000; 
	uint32_t spr_color = 0xFF000000;
	
	if (currentSelect != displaySettings) {
		for (uint16_t i = 0; i < courusel.size(); i++) {
			int16_t position_x = courusel[i]->x_pos+courusel[i]->image->x;
			if (position_x < - 88) position_x += max_length_pix;
			else if (position_x > max_length_pix - 88) position_x -= max_length_pix;
			if (position_x <  0 - courusel[i]->image->width || position_x > SCREEN_SIZE) continue;
			for (int16_t y = courusel[i]->y_pos+courusel[i]->image->y; y < courusel[i]->y_pos+courusel[i]->image->y+courusel[i]->image->height; y++)
			for (int16_t x = position_x; x < position_x+courusel[i]->image->width; x++) {
				if (x >= SCREEN_SIZE) break;
				if (x < 0) continue;
				uint32_t pixel = (y - courusel[i]->y_pos-courusel[i]->image->y) * courusel[i]->image->width + (x - position_x); 
				txt_color = (courusel[i]->image->rawData[pixel].R << 16) | (courusel[i]->image->rawData[pixel].G << 8) | courusel[i]->image->rawData[pixel].B;
				PutPixel(x, y, txt_color);
			}
		}

		if (currentSelect == gamelist || currentSelect == menu || currentSelect == preparegame)
		for (uint8_t i = 0; i < navigate.size(); i++) {
			int16_t position_y = navigate[i]->y_pos;
			int16_t position_x = navigate[i]->x_pos + navigate_offset;
			if (position_x <  0 - navigate[i]->image->width || position_x > SCREEN_SIZE) continue;
			for (int16_t y = position_y; y < position_y + navigate[i]->image->height; y++)
			for (int16_t x = position_x; x < position_x + navigate[i]->image->width; x++) {
				if (x >= SCREEN_SIZE) break;
				if (x < 0) continue;
				uint32_t pixel = (y - position_y) * navigate[i]->image->width + (x - position_x);
				txt_color = (navigate[i]->image->rawData[pixel].R << 16) | (navigate[i]->image->rawData[pixel].G << 8) | navigate[i]->image->rawData[pixel].B;
				PutPixel(x, y, txt_color);
			}
		}
	}

	for (uint8_t s = 0; s < sp_index; s++) {
		if (OAM[s].image) {
			for (uint8_t sy = 0; sy< OAM[s].image->height; sy++) 
			for (uint8_t sx = 0; sx< OAM[s].image->crop_width; sx++) {
				uint16_t pindex = sy * OAM[s].image->width + sx;
				spr_color = (OAM[s].image->rawData[pindex].R << 16) | (OAM[s].image->rawData[pindex].G << 8) | OAM[s].image->rawData[pindex].B;
				if (OAM[s].opacity < 100) spr_color = avrColor(OAM[s].opacity, 100-OAM[s].opacity, spr_color, GetPixel(OAM[s].x+OAM[s].image->x+sx, OAM[s].y+OAM[s].image->y+sy));
				PutPixel(OAM[s].x+OAM[s].image->x+sx, OAM[s].y+OAM[s].image->y+sy, spr_color);
				pindex++;
			}
			OAM[s].image = NULL;
		} else {
			for (int16_t y = OAM[s].y; y < OAM[s].y+8; y++)
			for (int16_t x = OAM[s].x; x < OAM[s].x+8; x++) {
				uint16_t spr_pattern = 0;
				uint8_t spr_bit_mux = 0;		
				if (OAM[s].attribute&0x40) spr_pattern = sprites[(OAM[s].attribute&0x20)>>5].sprite[OAM[s].id].pattern[7-(y-OAM[s].y)];
									  else spr_pattern = sprites[(OAM[s].attribute&0x20)>>5].sprite[OAM[s].id].pattern[y-OAM[s].y];
				if (OAM[s].attribute&0x80) {
										spr_pattern >>= (x-OAM[s].x) << 1;
										spr_bit_mux = spr_pattern & 0x03;
									}  else {
										spr_pattern <<= (x-OAM[s].x) << 1;
										spr_bit_mux = (spr_pattern & 0xC000) >> 14;
									}
				spr_color = pallete[OAM[s].attribute&0x1F].color[spr_bit_mux];			
				if (!(spr_color&0xFF000000)) {
					if (OAM[s].opacity < 100) spr_color = avrColor(OAM[s].opacity, 100-OAM[s].opacity, spr_color, GetPixel(x, y));
					PutPixel(x, y, spr_color);
				}
			}
		}
	}
}