/*----TO-DO list----*/
/* Утечка памяти при выходе из эмулятора, память не доконца очищается */
/* На карточке игры должна рисоваться маленькая пиктограмма если есть временное сохранение */ 

#include "Shell.h"
#include <cstdio>
#include <string>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include "LuPng/lupng.h"

/*
uint16_t RGB888ToRGB565( uint32_t color )
{
	return (color>>8&0xf800)|(color>>5&0x07e0)|(color>>3&0x001f);
}
*/


namespace fs = std::filesystem;

uint8_t sp_index = 0;
uint64_t clock_counter;
uint16_t sel = 0;
uint16_t game_count = 0;
uint8_t sortType = 0;

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
	LoadSettings();
}

void Shell::loadBitmap(const char* fname, Image * image) {
	#ifdef INFO
	printf("INFO: Load PNG File: %s\n", fname);
	#endif
	LuImage *img;
	if (img = luPngReadFile(fname)) {
		image->width = img->width;
		image->height = img->height;
		uint32_t sizeRaw = img->width * img->height;
		image->rawData.resize(sizeRaw);
        for (int p = 0, i = 0; i < img->dataSize && p < sizeRaw; i += 3) {
        	image->rawData[p].R = img->data[i];
            image->rawData[p].G = img->data[i + 1];
            image->rawData[p].B = img->data[i + 2];
            p++;
        }
       luImageRelease(img, NULL);
	} else {
		#ifdef INFO
		printf("ERROR: Can't open file: %s\n", fname);
		#endif
	}
}

void Shell::SaveSettings() {
	#ifdef INFO
	printf("INFO: %s\n", "Save settings.");
	#endif
	FILE* fb = fopen("setting.cfg", "wb");
	if (fb) {
		fwrite(&setting, sizeof(Settings), 1, fb);
	} else {
		#ifdef INFO
		printf("ERROR: %s\n", "Can't create save file.");
		#endif	
	}
}

void Shell::LoadSettings() {
	#ifdef INFO
	printf("INFO: %s\n", "Load settings.");
	#endif
	FILE* fb = fopen("setting.cfg", "rb");
	if (fb) {
		fread(&setting, sizeof(Settings), 1, fb);
	} else {
		#ifdef INFO
		printf("ERROR: %s\n", "Can't load save file.");
		#endif	
	}
}


void Shell::SaveSaves() {
	if (!courusel[sel]) return;
	FILE* fb = fopen((courusel[sel]->path + "\\gamesaves.sav").c_str(), "wb");
	if (fb) {
		for (uint8_t i = 0; i<4; i++) {
			if (courusel[sel]->savePointList[i]) {
				fwrite(&i, sizeof(uint8_t), 1, fb);
				fwrite(&courusel[sel]->savePointList[i]->time_shtamp, sizeof(uint16_t), 1, fb);
				if (courusel[sel]->savePointList[i]->image) {
					uint32_t length = 7 + sizeof(BmpPixel) * courusel[sel]->savePointList[i]->image->rawData.size();
					fwrite(&length, sizeof(uint32_t), 1, fb);
					fwrite(&courusel[sel]->savePointList[i]->image->width, sizeof(uint8_t), 1, fb);
					fwrite(&courusel[sel]->savePointList[i]->image->height, sizeof(uint8_t), 1, fb);
					fwrite(&courusel[sel]->savePointList[i]->image->crop_width, sizeof(uint8_t), 1, fb);
					fwrite(&courusel[sel]->savePointList[i]->image->x, sizeof(uint16_t), 1, fb);
					fwrite(&courusel[sel]->savePointList[i]->image->y, sizeof(uint16_t), 1, fb);
					fwrite(courusel[sel]->savePointList[i]->image->rawData.data(), sizeof(BmpPixel), courusel[sel]->savePointList[i]->image->rawData.size(), fb);
				} else fwrite(0x00, sizeof(uint32_t), 1, fb);
				
				if (courusel[sel]->savePointList[i]->state) {
					uint32_t mapStateSize = courusel[sel]->savePointList[i]->state->CART.sizeMapState;
					uint32_t CHRamSize = courusel[sel]->savePointList[i]->state->CART.maxCHRAddr;
					uint32_t PRGRamSize = courusel[sel]->savePointList[i]->state->CART.maxPRGAddr;

					uint32_t length = 0x800 + 4 + sizeof(CPUState) + sizeof(PPUState) + sizeof(APUState) + 
					2 + 2 + 4 + mapStateSize + 4 + CHRamSize + 4 + PRGRamSize;

					fwrite(&length, sizeof(uint32_t), 1, fb);

					fwrite(&courusel[sel]->savePointList[i]->state->RAM, sizeof(uint8_t), 0x800, fb);
					
					fwrite(&courusel[sel]->savePointList[i]->state->nSystemClockCounter, sizeof(uint32_t), 1, fb);

					fwrite(&courusel[sel]->savePointList[i]->state->CPU, sizeof(CPUState), 1, fb);
					fwrite(&courusel[sel]->savePointList[i]->state->PPU, sizeof(PPUState), 1, fb);
					fwrite(&courusel[sel]->savePointList[i]->state->APU, sizeof(APUState), 1, fb);

					fwrite(&courusel[sel]->savePointList[i]->state->CART.maxCHRAddr, sizeof(uint16_t), 1, fb);
					fwrite(&courusel[sel]->savePointList[i]->state->CART.maxPRGAddr, sizeof(uint16_t), 1, fb);
					
					fwrite(&mapStateSize, sizeof(uint32_t), 1, fb);
					if (mapStateSize) {
						fwrite(courusel[sel]->savePointList[i]->state->CART.mapperState, sizeof(uint8_t), mapStateSize, fb);
					}
					
					fwrite(&CHRamSize, sizeof(uint32_t), 1, fb);
					if (CHRamSize) {
						fwrite(courusel[sel]->savePointList[i]->state->CART.CHRam, sizeof(uint8_t), CHRamSize, fb);
					}
					
					fwrite(&PRGRamSize, sizeof(uint32_t), 1, fb);
					if (PRGRamSize) {
						fwrite(courusel[sel]->savePointList[i]->state->CART.PRGRam, sizeof(uint8_t), PRGRamSize, fb);
					}

				} else fwrite(0x00, sizeof(uint32_t), 1, fb);
				
			}
		}
		fclose(fb);
	}
}

void Shell::LoadSaves(SavePoint * savePointList[4], uint8_t id, const char* fname) {
	FILE* fb = fopen(fname, "rb");
	if (fb) {
		fseek(fb , 0, SEEK_SET);
		while (!feof(fb)) {
			uint8_t ndx;
			if (!fread(&ndx, sizeof(uint8_t), 1, fb)) break;
			if (ndx > 3 || savePointList[ndx]) {
				#ifdef INFO
				printf("ERROR: %d save state already exists or very large.\n", ndx);
				#endif
				break;
			}
			savePointList[ndx] = new SavePoint();
			savePointList[ndx]->id = id;
			fread(&savePointList[ndx]->time_shtamp, sizeof(uint16_t), 1, fb);
			uint32_t length;
			fread(&length, sizeof(uint32_t), 1, fb);
			if (length) {
				savePointList[ndx]->image = new Image();
				fread(&savePointList[ndx]->image->width, sizeof(uint8_t), 1, fb);
				fread(&savePointList[ndx]->image->height, sizeof(uint8_t), 1, fb);
				fread(&savePointList[ndx]->image->crop_width, sizeof(uint8_t), 1, fb);
				fread(&savePointList[ndx]->image->x, sizeof(uint16_t), 1, fb);
				fread(&savePointList[ndx]->image->y, sizeof(uint16_t), 1, fb);
				savePointList[ndx]->image->rawData.resize((length-7) / sizeof(BmpPixel));
				fread(savePointList[ndx]->image->rawData.data(), sizeof(BmpPixel), savePointList[ndx]->image->rawData.size(), fb);
			}
			
			fread(&length, sizeof(uint32_t), 1, fb);
			if (length) {
				savePointList[ndx]->state = new State();
				fread(&savePointList[ndx]->state->RAM, sizeof(uint8_t),  0x800, fb);
				fread(&savePointList[ndx]->state->nSystemClockCounter, sizeof(uint32_t), 1, fb);
				fread(&savePointList[ndx]->state->CPU, sizeof(CPUState), 1, fb);
				fread(&savePointList[ndx]->state->PPU, sizeof(PPUState), 1, fb);
				fread(&savePointList[ndx]->state->APU, sizeof(APUState), 1, fb);

				fread(&savePointList[ndx]->state->CART.maxCHRAddr, sizeof(uint16_t), 1, fb);
				fread(&savePointList[ndx]->state->CART.maxPRGAddr, sizeof(uint16_t), 1, fb);
				
				uint32_t mapStateSize;
				fread(&mapStateSize, sizeof(uint32_t), 1, fb);
				if (mapStateSize) {
					savePointList[ndx]->state->CART.sizeMapState = mapStateSize;
					savePointList[ndx]->state->CART.mapperState = new uint8_t[mapStateSize];
					fread(savePointList[ndx]->state->CART.mapperState, sizeof(uint8_t), mapStateSize, fb);
				} else {
					#ifdef INFO
					printf("ERROR: MapperStateSize = %d\n", mapStateSize);
					#endif
				}
				uint32_t CHRamSize;
				fread(&CHRamSize, sizeof(uint32_t), 1, fb);
				if (CHRamSize) {
					savePointList[ndx]->state->CART.CHRam = new uint8_t[CHRamSize];
					fread(savePointList[ndx]->state->CART.CHRam, sizeof(uint8_t), CHRamSize, fb);
				}
				uint32_t PRGRamSize;
				fread(&PRGRamSize, sizeof(uint32_t), 1, fb);
				if (PRGRamSize) {
					savePointList[ndx]->state->CART.PRGRam = new uint8_t[PRGRamSize];
					fread(savePointList[ndx]->state->CART.PRGRam, sizeof(uint8_t), PRGRamSize, fb);
				}
			}
		};
		fclose(fb);
	}	
}

void Shell::navigateRestructure() {
	max_length_navbar = 2;
	for (uint16_t i = 0; i < courusel.size(); i++) {
			courusel[i]->navCard->y_pos = 172 + (20-courusel[i]->navCard->image->height);
			courusel[i]->navCard->x_pos = max_length_navbar;
			max_length_navbar += courusel[i]->navCard->image->width+1;
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
	 				Card->path = entry.path().string();
					std::string icon = reader.Get("Desktop Entry", "Icon", "");
					Card->exec = reader.Get("Desktop Entry", "Exec", "");

					Card->name = reader.Get("Desktop Entry", "Name", "");
					if (Card->name.length()==0) Card->name = "Unknown";

					if (Card->name.length() > TBL_WIDTH - 10) {
						uint16_t l = (Card->name.length() - (TBL_WIDTH - 10));
						Card->name.erase(Card->name.length() - l - 3, l + 3);
						Card->name += "...";
					}

					Card->text = reader.Get("Description", "Text", "");
					if (Card->text.length()==0) Card->text = "No description";

					Card->date = reader.Get("X-CLOVER Game", "ReleaseDate", "");
					if (Card->date.length()==0) Card->date = "1970-01-01";

					Card->SortRawPublisher = reader.Get("X-CLOVER Game", "SortRawPublisher", "");
					if (Card->SortRawPublisher.length()==0) Card->SortRawPublisher = "Unknown";

					Card->SortRawTitle = reader.Get("X-CLOVER Game", "SortRawTitle", "");
					if (Card->SortRawTitle.length()==0) Card->SortRawTitle = "Unknown";

					Card->copyright = reader.Get("X-CLOVER Game", "Copyright", "");
					if (Card->copyright.length()==0) Card->copyright = "Unknown copyright";

					if (Card->copyright.length() > TBL_WIDTH - 4) {
						uint16_t l = (Card->copyright.length() - (TBL_WIDTH - 4));
						Card->copyright.erase(Card->copyright.length() - l - 3, l + 3);
						Card->copyright += "...";
					}

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
				
					Card->id = index++;
					Card->select = false;
					Card->pSelect = new bool;
					
					Card->navCard = new navcard;
					Card->navCard->id = Card->id;
					Card->navCard->select = Card->pSelect;
					Card->navCard->image = new Image();
					loadBitmap(icon.c_str(), Card->navCard->image);

					for (uint8_t i = 0; i<4; i++) Card->savePointList[i] = nullptr;
					LoadSaves(Card->savePointList, Card->id, (Card->path + "\\gamesaves.sav").c_str());

					courusel.push_back(Card);
	 			}

	game_count = courusel.size();
 	if (game_count > 0 && game_count * 10 <= TBL_WIDTH ) {
	 	uint16_t count = TBL_WIDTH / (game_count*10);
	 	game_count *= count+1;
 	}
 	if (courusel.size() == 0) {
 		currentSelect = empty;
 		#ifdef INFO
		printf("WARNING: %s\n", "Empty game!");
		#endif
 	}
 	else {
 		std::sort(courusel.begin(),courusel.end(), [](auto& l, auto& r){return ((card*)l)->SortRawTitle<((card*)r)->SortRawTitle;}); 
 		navigateRestructure();
 	}
 	navigate_offset = (SCREEN_SIZE - max_length_navbar) >> 1;
 	FILE* fp = fopen(table_sprites, "rb");
    if (!fp) return false;
    fread(&pallete[0], 0x200, 1, fp);
    fread(&sprites[0], 0x2000, 1, fp);
    fclose(fp);
	return true;
}

uint8_t l_y = 7;
void Shell::drawCourusel(uint8_t y, uint8_t counter) {
	#ifdef DEBUG 
	printf("DEBUG: %s.\n", "drawCourusel");
	#endif
	if (!courusel.size()) return;
	uint8_t h = 10;
	uint8_t w = 10;
	max_length = game_count * (w + 1);
	if (max_length < TBL_WIDTH+w+1) max_length = TBL_WIDTH+w+1;
	if (counter == 0) x_offset = (courusel_direction==0?x_offset:(courusel_direction<0?--x_offset:++x_offset))%max_length;
	int16_t x = x_offset;
	for (uint16_t ci = 0; ci < game_count; ci++) {
		uint16_t i = ci%courusel.size();
		courusel[i]->x_pos = ((i*(w+1)) + x) << 3;
		courusel[i]->y_pos = y << 3;
		int16_t bxs = (ci*(w+1)) + x;
		int16_t bs = bxs;
		if (bs < 0) bs += max_length;
		else if (bs > max_length-1) bs -= max_length;
		if (bs == select_to) courusel_direction = 0;
		courusel[i]->select = bs == stable_position;
		* courusel[i]->pSelect = false;
		if (courusel[i]->select) {
			sel = i;
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
				tblName[bn+v*TBL_WIDTH] = c==0?0x5B:(c==w-1?0x5C:(c==w-3&&v==h-1?(courusel[i]->players==1?0x8D:(courusel[i]->simultaneous==1?0x9D:0x8C)):(c==w-2&&v==h-1?(courusel[i]->simultaneous==1?0x9E:0x8E):(v==h-1&&c>0&&c<5?(courusel[i]->savePointList[c-1]?0x9F:0x8F):0x4B))));
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
	#ifdef DEBUG 
	printf("DEBUG: %s.\n", "drawNavigate");
	#endif
	if (!courusel.size()) return;
	if (!(currentSelect == gamelist || currentSelect == menu || currentSelect == preparegame)) return;
	uint16_t nav_sel = sel/*%game_count*/;
	if (!counter) cursor = (++cursor)%3;
	if (max_length_navbar > SCREEN_SIZE-4) {
		navigate_offset = 0 - ((float)(max_length_navbar - SCREEN_SIZE + 4 ) / courusel.size()) * nav_sel;		
		int16_t length_offset = courusel[nav_sel]->navCard->x_pos + navigate_offset + courusel[nav_sel]->navCard->image->width+1;		
		if (length_offset > SCREEN_SIZE-2)
		navigate_offset -= length_offset - SCREEN_SIZE  + 2;
	}
	current_pos_x = courusel[nav_sel]->navCard->x_pos + navigate_offset + (courusel[nav_sel]->navCard->image->width >> 1) - 8;
	current_pos_y = courusel[nav_sel]->navCard->y_pos;
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
	#ifdef DEBUG 
	printf("DEBUG: %s.\n", "drawTopBar");
	#endif
	for (uint8_t i = 0; i < 4; i++) {
		memset(&tblName[i*TBL_WIDTH], 0x10*i, TBL_WIDTH);
		if (i) memset(&tblAttribute[i*TBL_WIDTH + (HALF_TBL_WIDTH) - 8], 0x04, 16);
		else memset(&tblAttribute[i*TBL_WIDTH], 0x00, (TBL_WIDTH<<2));
	}
}

void Shell::drawDownBar() {
	#ifdef DEBUG 
	printf("DEBUG: %s.\n", "drawDownBar");
	#endif
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
	#ifdef DEBUG 
	printf("DEBUG: %s.\n", "drawTempSaveState");
	#endif
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
	if (currentSelect == saves && select_save_state < 0) {
		x = temp_save_state_x + 8;
		y = temp_save_state_y + 8;
		attr = 0x09;
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
			if (deleteSavePoint(temp_save_state)) temp_save_state = nullptr;
			animateboom = 0;
			select_save_state = 0;

			if (currentSelect == saves) {
				no_select_state = true;
				for (uint8_t i = 0; i <=maxSaveState; i++) {
					if (courusel[sel]->savePointList[i]) {
						select_to_syspend_panel_selector = 6 + 8*i;
						select_save_state = i;
						no_select_state = false;
						break;
					}
				}
			}

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
			if (select_save_state < maxSaveState) drawSprite(0xE8, x+(size+1)*8 + (blink?15:13),	4 + y+(size-1)*4, 0x19, 1, 2);
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
	uint8_t offset = select_to+4;
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
	if ((!temp_save_state && stable_position_syspend_panel_selector==6 + 8*(ndx-1))|
		(temp_save_state && temp_save_state->id != courusel[sel]->id)) {
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

void Shell::drawDisplayContent(uint8_t counter) {
	uint8_t length_items = 22;
	uint8_t offset = (TBL_WIDTH - length_items) >> 1;
	for (uint8_t i = 0; i<5; i++)
	memset(&tblAttribute[(i+8+6*displaySettingSelector)*TBL_WIDTH + offset], 0x1A, length_items);

	tblName[10*TBL_WIDTH + offset + 3 ] = setting.disaplayScale == 0?0xA8:0xA7;
	memcpy(&tblName[10*TBL_WIDTH + offset + 5], "CRT filter", 10);
	memset(&tblAttribute[10*TBL_WIDTH + offset + 5], displaySettingSelector==0?0x3A:0x27, 10);

	tblName[16*TBL_WIDTH + offset + 3 ] = setting.disaplayScale == 1?0xA8:0xA7;
	memcpy(&tblName[16*TBL_WIDTH + offset + 5], "4:3", 3);
	memset(&tblAttribute[16*TBL_WIDTH + offset + 5], displaySettingSelector==1?0x3A:0x27, 3);

	tblName[22*TBL_WIDTH + offset + 3] = setting.disaplayScale == 2?0xA8:0xA7;
	memcpy(&tblName[22*TBL_WIDTH + offset + 5], "Pixel Perfect", 13);
	memset(&tblAttribute[22*TBL_WIDTH + offset + 5], displaySettingSelector==2?0x3A:0x27, 13);

	if (!counter) cursor = (++cursor)%3;
	for (uint8_t i = 0; i<2; i++) {
		tblName[(7+6*displaySettingSelector)*TBL_WIDTH + HALF_TBL_WIDTH + i - 1] =  0xBD+cursor;
		tblAttribute[(7+6*displaySettingSelector)*TBL_WIDTH + HALF_TBL_WIDTH + i - 1]  = !i?0x9B:0x1B;
	}
}

void Shell::drawOptionsContent(uint8_t counter) {
	uint8_t swtch [2][3] = {{0xAA,0xAB,0xAC},{0xBC,0xBB,0xBA}};
	uint8_t swtch_mask [2][4][3] = {{{0x1B,0x1B,0x1B},{0x5B,0x5B,0x5B},{0xDB,0xDB,0xDB},{0x9B,0x9B,0x9B}},{{0x1A,0x1A,0x1A},{0x5A,0x5A,0x5A},{0xDA,0xDA,0xDA},{0x9A,0x9A,0x9A}}};
	const char * text[11] = {"  0%", " 10%", " 20%", " 30%" , " 40%", " 50%", " 60%", " 70%", " 80%", " 90%", "100%"};
	uint8_t length_items = 22;
	uint8_t offset = (TBL_WIDTH - length_items) >> 1;
	for (uint8_t i = 0; i<4; i++)
	memset(&tblAttribute[(7 + i + 4*optionSettingSelector)*TBL_WIDTH + offset], 0x1A, length_items);

	memcpy(&tblName[8*TBL_WIDTH + offset + 1], "Volume", 6);
	memset(&tblAttribute[8*TBL_WIDTH + offset + 1], optionSettingSelector==0?0x3A:0x27, 6);
	for (uint8_t i = 0; i<10; i++) memset(&tblName[9*TBL_WIDTH + offset + 1 + i], (i<setting.volume?0xA9:0x99), 1);	memcpy(&tblName[9*TBL_WIDTH + offset + length_items-5], text[setting.volume], 4);	
	memset(&tblAttribute[9*TBL_WIDTH + offset + 1], optionSettingSelector==0?0x1A:0x07, 10);		memset(&tblAttribute[9*TBL_WIDTH + offset + length_items-5], optionSettingSelector==0?0x3A:0x27, 4);

	memcpy(&tblName[12*TBL_WIDTH + offset + 1], "Brightness", 10);
	memset(&tblAttribute[12*TBL_WIDTH + offset + 1], optionSettingSelector==1?0x3A:0x27, 10);
	for (uint8_t i = 0; i<10; i++) memset(&tblName[13*TBL_WIDTH + offset + 1 + i], (i<setting.brightness?0xA9:0x99), 1); memcpy(&tblName[13*TBL_WIDTH + offset + length_items-5], text[setting.brightness], 4);	
	memset(&tblAttribute[13*TBL_WIDTH + offset + 1], optionSettingSelector==1?0x1A:0x07, 10);		memset(&tblAttribute[13*TBL_WIDTH + offset + length_items-5], optionSettingSelector==1?0x3A:0x27, 4);

	memcpy(&tblName[16*TBL_WIDTH + offset + 1], "Playing", 7);										memcpy(&tblName[16*TBL_WIDTH + offset + length_items-4], &swtch[setting.play_home_music?1:0], 3); 
	memset(&tblAttribute[16*TBL_WIDTH + offset + 1], optionSettingSelector==2?0x3A:0x27, 7);		memcpy(&tblAttribute[16*TBL_WIDTH + offset + length_items-4], &swtch_mask[optionSettingSelector==2?1:0][setting.play_home_music?2:0], 3);
	memcpy(&tblName[17*TBL_WIDTH + offset + 1], "home music", 10);									memcpy(&tblName[17*TBL_WIDTH + offset + length_items-4], &swtch[setting.play_home_music?1:0], 3);
	memset(&tblAttribute[17*TBL_WIDTH + offset + 1], optionSettingSelector==2?0x3A:0x27, 10);		memcpy(&tblAttribute[17*TBL_WIDTH + offset + length_items-4], &swtch_mask[optionSettingSelector==2?1:0][setting.play_home_music?3:1], 3);

	memcpy(&tblName[20*TBL_WIDTH + offset + 1], "Enable", 6);										memcpy(&tblName[20*TBL_WIDTH + offset + length_items-4], &swtch[setting.turbo_a?1:0], 3); 
	memset(&tblAttribute[20*TBL_WIDTH + offset + 1], optionSettingSelector==3?0x3A:0x27, 6);		memcpy(&tblAttribute[20*TBL_WIDTH + offset + length_items-4], &swtch_mask[optionSettingSelector==3?1:0][setting.turbo_a?2:0], 3);
	memcpy(&tblName[21*TBL_WIDTH + offset + 1], "TURBO A", 7);										memcpy(&tblName[21*TBL_WIDTH + offset + length_items-4], &swtch[setting.turbo_a?1:0], 3); 
	memset(&tblAttribute[21*TBL_WIDTH + offset + 1], optionSettingSelector==3?0x3A:0x27, 7);		memcpy(&tblAttribute[21*TBL_WIDTH + offset + length_items-4], &swtch_mask[optionSettingSelector==3?1:0][setting.turbo_a?3:1], 3);

	memcpy(&tblName[24*TBL_WIDTH + offset + 1], "Enable", 6);										memcpy(&tblName[24*TBL_WIDTH + offset + length_items-4], &swtch[setting.turbo_b?1:0], 3); 
	memset(&tblAttribute[24*TBL_WIDTH + offset + 1], optionSettingSelector==4?0x3A:0x27, 6);		memcpy(&tblAttribute[24*TBL_WIDTH + offset + length_items-4], &swtch_mask[optionSettingSelector==4?1:0][setting.turbo_b?2:0], 3);
	memcpy(&tblName[25*TBL_WIDTH + offset + 1], "TURBO B", 7);										memcpy(&tblName[25*TBL_WIDTH + offset + length_items-4], &swtch[setting.turbo_b?1:0], 3); 
	memset(&tblAttribute[25*TBL_WIDTH + offset + 1], optionSettingSelector==4?0x3A:0x27, 7);		memcpy(&tblAttribute[25*TBL_WIDTH + offset + length_items-4], &swtch_mask[optionSettingSelector==4?1:0][setting.turbo_b?3:1], 3);

	if (!counter) cursor = (++cursor)%3;
	for (uint8_t i = 0; i<2; i++) {
		tblName[(	  8+i  + 4*optionSettingSelector)*TBL_WIDTH + offset - 2] =  0xAD+cursor;
		tblAttribute[(8+i  + 4*optionSettingSelector)*TBL_WIDTH + offset - 2]  = !i?0x1B:0x5B;
	}
}
uint8_t HintTimer;
void Shell::drawHintWindow(uint8_t counter) {
	if (HintTimer == 0) return;
	if (counter == 0) HintTimer--;
	std::string title = "";
	switch (sortType) {
		case 0: title = "By title"; break;
		case 1: title = "By 2-player games"; break;
		case 2: title = "By release date"; break;
		case 3: title = "By publisher"; break;
	}
	uint8_t offset = TBL_WIDTH - title.length() - 3;
	memset(&tblName[5 * TBL_WIDTH + offset],  0x94, 1);memset(&tblName[5 * TBL_WIDTH + offset+ 1],  0x95, title.length());memset(&tblName[5 * TBL_WIDTH + offset + 1 + title.length()],  0x94, 1);
	memset(&tblAttribute[5 * TBL_WIDTH + offset],  0x07, title.length()+1); 						 memset(&tblAttribute[5 * TBL_WIDTH + offset + 1 + title.length()],  0x07|0x80, 1);
	memset(&tblName[6 * TBL_WIDTH + offset],  0xA4, 1);memcpy(&tblName[6 * TBL_WIDTH + offset + 1], title.c_str(), title.length());memset(&tblName[6 * TBL_WIDTH + offset + 1 + title.length()],  0xA4, 1);
	memset(&tblAttribute[6 * TBL_WIDTH + offset],  0x07, 1);memset(&tblAttribute[6 * TBL_WIDTH + offset + 1],  0x27, title.length());memset(&tblAttribute[6 * TBL_WIDTH + offset + 1 + title.length()],  0x07, 1);
	memset(&tblName[7 * TBL_WIDTH + offset],  0x94, 1);memset(&tblName[7 * TBL_WIDTH + offset + 1],  0x95, title.length());memset(&tblName[7 * TBL_WIDTH + offset + 1 + title.length()],  0x94, 1);
	memset(&tblAttribute[7 * TBL_WIDTH + offset],  0x07|0x40, title.length()+1);	 memset(&tblAttribute[7 * TBL_WIDTH + offset + 1 + title.length()],  0x07|0xC0, 1);
}


uint8_t scroll = 0;
bool scroll_end = false;
void Shell::drawAboutContent(uint8_t counter) {
	memcpy(&tblName[6*TBL_WIDTH + 2], courusel[sel]->copyright.c_str(), courusel[sel]->copyright.length());
	memset(&tblAttribute[6*TBL_WIDTH + 2], 0x27, courusel[sel]->copyright.length());

	memcpy(&tblName[7*TBL_WIDTH + 2], courusel[sel]->date.c_str(), courusel[sel]->date.length());
	memset(&tblAttribute[7*TBL_WIDTH + 2], 0x27, courusel[sel]->date.length());

	uint16_t start_position = 0;
	uint16_t end_position = TBL_WIDTH-4;
	std::string text = "";
	uint8_t scroll_count = 0;
	scroll_end = false;
	for (uint8_t i = 0; i < 19; i++) {
		do {
			if (courusel[sel]->text.length() - start_position >= TBL_WIDTH-4) text = courusel[sel]->text.substr(start_position,  TBL_WIDTH-4);
			else {
				end_position = courusel[sel]->text.length();
				text = courusel[sel]->text.substr(start_position,  courusel[sel]->text.length() - start_position);
			}	

			start_position = end_position;
			
			if (end_position < courusel[sel]->text.length()) {
				if (courusel[sel]->text[end_position] != ' ') {
					while (courusel[sel]->text[start_position] != ' ' && text.length() > (end_position - start_position)) start_position--;
				}
				text = text.substr(0, text.length() - (end_position - start_position));
			} else { scroll_end = true; break; }

			do {
				start_position += 1; 
			} while (courusel[sel]->text[start_position] == ' ');
			end_position = start_position + (TBL_WIDTH-4);

		} while (scroll > scroll_count++ && end_position <= courusel[sel]->text.length());
		memcpy(&tblName[(i+9)*TBL_WIDTH + 2], text.c_str(), text.length());
		memset(&tblAttribute[(i+9)*TBL_WIDTH + 2], 0x27, text.length());
		if (scroll_end) break;
	}
}


void Shell::drawManualsContent(uint8_t counter) {
	uint8_t qr  [18][18] = {	
		{0xFC,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA},
		{0xFB,0xFC,0xFA,0xFA,0xFB,0xFB,0xFC,0xF9,0xFD,0xF9,0xFC,0xF9,0xFD,0xF9,0xFC,0xFA,0xFA,0xFB},
		{0xFB,0xFB,0x4B,0xFB,0xFB,0xFC,0xFC,0x09,0xFA,0xF9,0xFC,0xFA,0xFC,0xF9,0xFB,0x4B,0xFB,0xFB},
		{0xFB,0xFB,0xFA,0xF9,0xFB,0xFB,0xF9,0xFC,0xFC,0xFD,0xFA,0xFD,0xF9,0xFB,0xFB,0xFA,0xF9,0xFB},
		{0xFB,0xFA,0xFA,0xFA,0xF9,0xFB,0xFD,0xF9,0xF9,0xFB,0xFD,0xFC,0xFB,0xFB,0xFA,0xFA,0xFA,0xF9},
		{0xFB,0xF9,0xFA,0xFC,0xFD,0xF9,0x09,0x4B,0x09,0xFC,0xFC,0x09,0xFC,0xF9,0x4B,0xFC,0xFB,0xF9},
		{0xFB,0xFA,0xFD,0xF9,0xFC,0xFC,0xF9,0xFB,0xFA,0xFC,0xFA,0x4B,0xF9,0xFC,0xFB,0xFC,0x4B,0x09},
		{0xFB,0xFC,0xFC,0xFC,0xFA,0xFA,0xFA,0xF9,0xFC,0xFD,0xFD,0xF9,0x4B,0xF9,0xFC,0xFC,0xF9,0xF9},
		{0xFB,0xFC,0xFB,0xFC,0xFC,0x4B,0xFC,0xFD,0xFC,0xFA,0x09,0xFD,0xF9,0xFB,0xFC,0xFD,0xFD,0x09},
		{0xFB,0xFA,0xF9,0xF9,0xFD,0xFC,0xFD,0xFC,0xFC,0xFD,0xFC,0xFA,0xFC,0x4B,0xFC,0xFC,0xF9,0xFB},
		{0xFB,0xFB,0xFC,0xFC,0xFD,0xFB,0xFC,0xFC,0xF9,0xFB,0xF9,0xFC,0xFD,0xF9,0xFC,0xFC,0x4B,0x09},
		{0xFB,0xFA,0xFC,0xFD,0xFC,0xFA,0xFB,0xF9,0xF9,0xFA,0xF9,0xFA,0xFD,0xFA,0xF9,0xFC,0xFD,0xFB},
		{0xFB,0xFB,0xFC,0xFD,0xFD,0xF9,0xFD,0xF9,0xF9,0xFD,0xFC,0xFC,0xF9,0xFC,0xFD,0xFC,0x4B,0x09},
		{0xFB,0xF9,0x09,0xFA,0xF9,0xF9,0xFC,0xFD,0xF9,0xFB,0xFC,0xF9,0xFD,0xFC,0xFA,0xFB,0xF9,0xF9},
		{0xFB,0xFC,0xFA,0xFA,0xFB,0xFD,0xF9,0xFA,0xFC,0xFB,0xFA,0xFC,0x4B,0xFB,0xF9,0xFC,0x4B,0xF9},
		{0xFB,0xFB,0x4B,0xFB,0xFB,0xFC,0xF9,0xFC,0xFC,0xFD,0xFD,0xF9,0xFD,0xFC,0xFA,0xFC,0xFC,0xFB},
		{0xFB,0xFB,0xFA,0xF9,0xFB,0xFD,0xFD,0xF9,0xFB,0xFA,0xFB,0xFC,0xFC,0xFC,0xFC,0xFC,0xFB,0x09},
		{0xFB,0xFA,0xFA,0xFA,0xF9,0xFA,0xF9,0xFA,0xFA,0xFA,0xF9,0x09,0xFA,0xF9,0xFA,0x09,0xF9,0x09},
	};
	uint8_t mask [18][18] = {
		{0x97,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17},
		{0x97,0x97,0x17,0x17,0x97,0x17,0x17,0x97,0x97,0x97,0x97,0x97,0x17,0xD7,0x97,0x17,0x17,0x97},
		{0x97,0x97,0x17,0x97,0x97,0xD7,0x57,0x17,0x17,0x97,0x57,0x17,0x57,0xD7,0x97,0x17,0x97,0x97},
		{0x97,0x97,0x17,0x97,0x97,0x97,0xD7,0xD7,0x97,0x97,0x57,0x97,0x17,0x97,0x97,0x17,0x97,0x97},
		{0x97,0x17,0x17,0x17,0x97,0x97,0x97,0x97,0x97,0x97,0x97,0xD7,0x97,0x97,0x17,0x17,0x17,0x97},
		{0x97,0x97,0x17,0x97,0x97,0xD7,0x17,0x17,0x17,0x57,0x57,0x17,0x17,0x97,0x17,0x17,0x97,0xD7},
		{0x97,0x57,0x97,0xD7,0x17,0xD7,0x97,0x17,0x17,0x17,0x57,0x17,0x57,0x17,0x97,0xD7,0x17,0x17},
		{0x97,0x57,0x57,0x17,0x17,0x57,0x17,0xD7,0x17,0x97,0x17,0x97,0x17,0x17,0x57,0x17,0x57,0xD7},
		{0x97,0x57,0x97,0x97,0x17,0x17,0x57,0x97,0x17,0x17,0x17,0x97,0x97,0x17,0xD7,0x97,0x17,0x17},
		{0x97,0x57,0xD7,0x17,0x97,0x57,0x97,0x97,0x97,0x17,0xD7,0x57,0xD7,0x17,0x57,0x17,0xD7,0x97},
		{0x97,0x17,0x57,0xD7,0x97,0x17,0x17,0xD7,0x17,0x97,0x97,0x57,0x97,0x57,0x17,0xD7,0x17,0x17},
		{0x97,0x57,0x97,0x17,0x17,0x57,0x97,0x97,0x57,0x57,0x97,0x57,0x97,0x17,0xD7,0x17,0x17,0x97},
		{0x97,0x97,0x17,0x17,0x97,0x97,0x17,0xD7,0xD7,0x17,0x57,0x57,0x97,0x17,0x17,0x57,0x17,0x17},
		{0x97,0x97,0x17,0x17,0x97,0xD7,0x97,0x17,0xD7,0x17,0x57,0xD7,0x97,0x97,0x17,0x97,0xD7,0xD7},
		{0x97,0x97,0x17,0x17,0x97,0x17,0x17,0x17,0x17,0x17,0x57,0x57,0x17,0x97,0x97,0xD7,0x17,0xD7},
		{0x97,0x97,0x17,0x97,0x97,0xD7,0xD7,0x17,0x57,0x97,0x17,0x57,0x97,0x17,0x17,0x97,0x57,0x97},
		{0x97,0x97,0x17,0x97,0x97,0x97,0x97,0xD7,0x17,0x17,0x97,0x57,0x97,0x17,0x97,0x57,0x97,0x17},
		{0x97,0x17,0x17,0x17,0x97,0x17,0x97,0x17,0x17,0x17,0x17,0x17,0x17,0x97,0x17,0x17,0x17,0x17},
	};
	uint8_t offset = (TBL_WIDTH - 30) >> 1;
	uint8_t qr_offset = (TBL_WIDTH - 18) >> 1;
	memcpy(&tblName[5*TBL_WIDTH + offset], "View the original game manuals", 30);
	memset(&tblAttribute[5*TBL_WIDTH + offset], 0x27, 30);
	memcpy(&tblName[6*TBL_WIDTH + offset], "and other information on your ", 30);
	memset(&tblAttribute[6*TBL_WIDTH + offset], 0x27, 30);
	memcpy(&tblName[7*TBL_WIDTH + offset], "         smart device.        ", 30);
	memset(&tblAttribute[7*TBL_WIDTH + offset], 0x27, 30);
	for(uint8_t i = 0; i<18; i++) {
		memcpy(&tblName[(8+i)*TBL_WIDTH + qr_offset], &qr[i], 18);
		memcpy(&tblAttribute[(8+i)*TBL_WIDTH + qr_offset], &mask[i], 18);
	}
	memcpy(&tblName[27*TBL_WIDTH + offset], "www.nintendo.co.jp/clv/manuals", 30);
	memset(&tblAttribute[27*TBL_WIDTH + offset], 0x27, 30);

}

void Shell::drawPanel(){
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
}

void Shell::drawManualsPanel(uint8_t counter) {	
	drawPanel();
	drawSprite(0x69, 11, 8, 0x0B, 3);
	memcpy(&tblName[(TBL_WIDTH<<1) + 5], "Manuals", 7);
	memset(&tblAttribute[(TBL_WIDTH<<1) + 5], 0x27, 7);
	drawManualsContent(counter);
}

void Shell::drawAboutPanel(uint8_t counter) {	
	drawPanel();
	drawSprite(0x66, 11, 8, 0x07, 3);
	memcpy(&tblName[(TBL_WIDTH<<1) + 5], "About", 5);
	memset(&tblAttribute[(TBL_WIDTH<<1) + 5], 0x27, 5);
	drawAboutContent(counter);
}

void Shell::drawDisplayPanel(uint8_t counter) {	
	drawPanel();
	drawSprite(0x60, 11, 8, 0x07, 3);
	memcpy(&tblName[(TBL_WIDTH<<1) + 5], "Display", 7);
	memset(&tblAttribute[(TBL_WIDTH<<1) + 5], 0x27, 7);
	drawDisplayContent(counter);
}

void Shell::drawOptionsPanel(uint8_t counter) {	
	drawPanel();
	drawSprite(0x63, 11, 8, 0x07, 3);
	memcpy(&tblName[(TBL_WIDTH<<1) + 5], "Options", 7);
	memset(&tblAttribute[(TBL_WIDTH<<1) + 5], 0x27, 7);
	drawOptionsContent(counter);
}




void Shell::drawTitleBar(uint8_t x, uint8_t y, uint8_t l) {
	#ifdef DEBUG 
	printf("DEBUG: %s.\n", "drawTitleBar");
	#endif
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
	#ifdef DEBUG 
	printf("DEBUG: %s.\n", "drawHintBar");
	#endif
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
				memcpy(&tblName[27*TBL_WIDTH+offset], "Pick", 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 4);
				offset+=4+add;
				memcpy(&tblName[27*TBL_WIDTH+offset], &start, 3);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x0F, 3);
				offset+=3;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Game", 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 4);
				offset+=4+add;
				tblName[27*TBL_WIDTH+offset] = 0x51;
				tblAttribute[27*TBL_WIDTH+offset] = 0x0F;
				offset+=1;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Remove", 6);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 6);
				offset+=6+add;
				tblName[27*TBL_WIDTH+offset] = 0x50;
				tblAttribute[27*TBL_WIDTH+offset] = 0x0F;
				offset+=1;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Save", 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 4);
			} else {
				offset = (TBL_WIDTH-(24+add+add+add)) >> 1;
				tblName[27*TBL_WIDTH+offset] = 0x7C;
				tblAttribute[27*TBL_WIDTH+offset] = 0x0F;
				offset+=1;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Pick", 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 4);
				offset+=4+add;
				memcpy(&tblName[27*TBL_WIDTH+offset], &select, 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x0F, 4);
				offset+=4;
				memcpy(&tblName[27*TBL_WIDTH+offset], "Move", 4);
				memset(&tblAttribute[27*TBL_WIDTH+offset], 0x2F, 4);
				offset+=4+add;
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
		case display:
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
		case options:
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
		case about:
				offset = TBL_WIDTH - 1 - 14;
				tblName[2*TBL_WIDTH+offset] = 0xA6;
				offset+=1;
				memcpy(&tblName[2*TBL_WIDTH+offset], "Scroll", 6);
				memset(&tblAttribute[2*TBL_WIDTH+offset], 0x27, 6);
				offset+=7;
				tblName[2*TBL_WIDTH+offset] = 0x98;
				offset+=1;
				memcpy(&tblName[2*TBL_WIDTH+offset], "Back", 4);
				memset(&tblAttribute[2*TBL_WIDTH+offset], 0x27, 4);
		break;
		case manuals:
				offset = TBL_WIDTH - 1 - 6;
				tblName[2*TBL_WIDTH+offset] = 0x98;
				offset+=1;
				memcpy(&tblName[2*TBL_WIDTH+offset], "Back", 4);
				memset(&tblAttribute[2*TBL_WIDTH+offset], 0x27, 4);
		break;
	}
}

void Shell::drawSelector(selectors mode) {
	#ifdef DEBUG 
	printf("DEBUG: %s.\n", "drawSelector");
	#endif
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

bool Shell::deleteSavePoint(SavePoint * savepoint) {
	#ifdef DEBUG 
	printf("DEBUG: %s.\n", "deleteSavePoint");
	#endif
	if (savepoint) {
		if (savepoint->state->CART.CHRam) {
			delete savepoint->state->CART.CHRam;
			savepoint->state->CART.CHRam = nullptr;
		}
		if (savepoint->state->CART.PRGRam) {
			delete savepoint->state->CART.PRGRam;
			savepoint->state->CART.PRGRam = nullptr;
		}
		if (savepoint->state->CART.mapperState) {
			delete savepoint->state->CART.mapperState;
			savepoint->state->CART.mapperState = nullptr;
		}
		if (savepoint->state) {
			delete savepoint->state;
			savepoint->state = nullptr;
		}
		if (savepoint->image) {
			delete savepoint->image;
			savepoint->image = nullptr;
		}
		delete savepoint;
		savepoint = nullptr;
	}
	if (savepoint) return false;
	else return true;
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
				temp_save_state->state = new State();
				NES.SaveState(temp_save_state->state);
				NES.RejectCartridge();
				delete CART;
				CART = nullptr;
				currentSelect = gamelist; // exit;
				return;
			}

            if (setting.turbo_b && controller&0x40) {
                if (lastkbState & 0x40) lastkbState &= 0xBF;
                else lastkbState = lastkbState |= 0x40;
                controller = (lastkbState&0x40) | (controller & 0xBF);
            }
            if (setting.turbo_a && controller&0x80) {
                if (lastkbState & 0x80) lastkbState &= 0x7F;
                else lastkbState = lastkbState |= 0x80;
                controller = (lastkbState&0x80) | (controller & 0x7F);
            } 

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
		return;
	}
	if (currentSelect == preparegame && fadeProcents == 100 && !temp_save_state) {
		#ifdef INFO
		printf("INFO: %s.\n", "Play game");
		#endif
		memset(FrameBuffer, 0x00, SCREEN_SIZE * 240 << 2);
		currentSelect = playgame;
		return;
	} 
	counter++;

/* play music */
#ifdef DEBUG
printf("DEBUG: %s.\n", "Playing sound clock");
#endif

	if (setting.play_home_music) {
		if (bgm_boot) {
			PlayWavClock(bgm_boot);
			if (feof(bgm_boot->file)) {
				fclose(bgm_boot->file);
				bgm_boot->file = NULL;
				delete bgm_boot;
				bgm_boot = NULL;
				
			}
		} else PlayWavClock(bgm_home, true);
	}
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
//if (counter%20==0) lastkbState = 0x00;
/*right*/if (controller&0x01) {
			switch (currentSelect) {
				case menu:
					if (!(lastkbState&0x01)/* || counter%10==0 */)
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
					if (!(lastkbState&0x01)/* || counter%20==0*/) {
						if (temp_save_state && courusel[sel]->id == temp_save_state->id) {
							if (select_save_state >=0 && select_save_state < maxSaveState) {
								select_save_state++;
								PlayWav(se_sys_cursor);
							}
						} else {
							if (select_save_state >= 0 && select_save_state+1 <= maxSaveState)
							for (uint8_t i = select_save_state+1; i <= maxSaveState; i++) {
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
				case options:
					if (!(lastkbState&0x01)) {
						switch (optionSettingSelector) {
							case 0: if (setting.volume < 10) {setting.volume++; PlayWav(se_sys_click);} break;
							case 1: if (setting.brightness < 10) {setting.brightness++; PlayWav(se_sys_click);} break;
							case 2: if (!setting.play_home_music) {setting.play_home_music = true; PlayWav(se_sys_click);} break;
							case 3: if (!setting.turbo_a) {setting.turbo_a = true; PlayWav(se_sys_click);} break;
							case 4: if (!setting.turbo_b) {setting.turbo_b = true; PlayWav(se_sys_click);} break;
						}
					}
				break;
			}
		}
/*left*/if (controller&0x02) {
			switch (currentSelect) {
				case menu:
					if (!(lastkbState&0x02)/* || counter%10==0 */)
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
					if (!(lastkbState&0x02)/* || counter%20==0*/) {
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
				case options:
					if (!(lastkbState&0x02)) {
						switch (optionSettingSelector) {
							case 0: if (setting.volume > 0) {setting.volume--; PlayWav(se_sys_click);} break;
							case 1: if (setting.brightness > 0) {setting.brightness--; PlayWav(se_sys_click);} break;
							case 2: if (setting.play_home_music) {setting.play_home_music = false; PlayWav(se_sys_click);} break;
							case 3: if (setting.turbo_a) {setting.turbo_a = false; PlayWav(se_sys_click);} break;
							case 4: if (setting.turbo_b) {setting.turbo_b = false; PlayWav(se_sys_click);} break;
						}
					}
				break;
			}
		}

/*select*/if (controller&0x20&& !(lastkbState&0x20)) {
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
				case gamelist:
					switch (sortType = (++sortType)%4) {
						case 0: std::sort(courusel.begin(),courusel.end(), [](auto& l, auto& r){return ((card*)l)->SortRawTitle<((card*)r)->SortRawTitle;}); navigateRestructure(); break;
						case 1: std::sort(courusel.begin(),courusel.end(), [](auto& l, auto& r){return (
							((card*)l)->players>((card*)r)->players || (
							((card*)l)->players==((card*)r)->players && (((card*)l)->simultaneous > ((card*)r)->simultaneous || (
							((card*)l)->simultaneous == ((card*)r)->simultaneous && ((card*)l)->SortRawTitle<((card*)r)->SortRawTitle
							))));});
						 navigateRestructure(); break;
						case 2: std::sort(courusel.begin(),courusel.end(), [](auto& l, auto& r){return ((card*)l)->date<((card*)r)->date;}); navigateRestructure(); break;				
						case 3: std::sort(courusel.begin(),courusel.end(), [](auto& l, auto& r){return ((card*)l)->SortRawPublisher<((card*)r)->SortRawPublisher;}); navigateRestructure(); break;
					}
					HintTimer = 2;
				break;
			}
		}

/*start*/if (controller&0x10&& !(lastkbState&0x10)) {
			switch (currentSelect) {
				case saves:
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
					clock_counter = 0;
					NES.PPU.setFrameBuffer(FrameBuffer);
					NES.PPU.setScale(SCREEN_SIZE, setting.disaplayScale);
					NES.Reset();					
					if (temp_save_state) {
						if (temp_save_state->id == courusel[sel]->id && currentSelect == saves) {
							clock_counter = (uint64_t) temp_save_state->time_shtamp * 1789773;
							NES.LoadState(temp_save_state->state);
							if (deleteSavePoint(temp_save_state)) temp_save_state = nullptr;
							select_save_state = 0;
						} else {
							select_save_state = -1;
							PlayWav(se_sys_smoke);
						}
					} else if (currentSelect == saves) {
						if (courusel[sel]->savePointList[select_save_state]) {
							clock_counter = (uint64_t) courusel[sel]->savePointList[select_save_state]->time_shtamp * 1789773;
							NES.LoadState(courusel[sel]->savePointList[select_save_state]->state);
						}
					}	
					currentSelect = preparegame;
				break;
			}
		}
/* B  */if (controller&0x40&& !(lastkbState&0x40)) {
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
				case menu:	currentSelect = courusel.size()?gamelist:empty;
							PlayWav(se_sys_cancel);
							break;
				case saves:
						if (temp_save_state && temp_save_state->id == courusel[sel]->id) {
							select_save_state = -1;
							PlayWav(se_sys_smoke);
							SaveSaves();
							break;
						} 
						currentSelect = gamelist;
						PlayWav(se_sys_cancel);
						break;
				case display: SaveSettings(); currentSelect = menu;	PlayWav(se_sys_cancel);	break;
				case options: SaveSettings(); currentSelect = menu;	PlayWav(se_sys_cancel);	break;
				case about: currentSelect = menu;	PlayWav(se_sys_cancel);	break;
				case manuals: currentSelect = menu;	PlayWav(se_sys_cancel);	break;
			}
		}
/* A  */if (controller&0x80 && !(lastkbState&0x80)) {
			switch (currentSelect) {
				case menu:
					switch (select_to_menu_position>>2) {
						case 0: currentSelect = display; break;
						case 1: currentSelect = options; break;
						case 2: if(!courusel.size()) break;
								currentSelect = about; scroll = 0; break;
						case 3: currentSelect = manuals; break;
					}
					PlayWav(se_sys_click);
				break;

				case display:
					setting.disaplayScale = displaySettingSelector;
					PlayWav(se_sys_click);
				break;

				case options:
					switch (optionSettingSelector) {
						case 0: setting.volume = (setting.volume+1)%11; PlayWav(se_sys_click); break;
						case 1: setting.brightness = (setting.brightness+1)%11; PlayWav(se_sys_click); break;
						case 2: setting.play_home_music = !setting.play_home_music; PlayWav(se_sys_click); break;
						case 3: setting.turbo_a = !setting.turbo_a; PlayWav(se_sys_click); break;
						case 4: setting.turbo_b = !setting.turbo_b; PlayWav(se_sys_click); break;
					}
				break;
		
				case about: break;
				case manuals: break;
				case saves:
					if (temp_save_state && courusel[sel]->id == temp_save_state->id) {
						if (courusel[sel]->savePointList[select_save_state]) {
							if (deleteSavePoint(courusel[sel]->savePointList[select_save_state])) 
								courusel[sel]->savePointList[select_save_state] = nullptr;
						}
						courusel[sel]->savePointList[select_save_state] = temp_save_state;
						stable_position_syspend_panel_selector = 6 + 8 * select_save_state;
						select_to_syspend_panel_selector = stable_position_syspend_panel_selector;
						temp_save_state = nullptr;
						no_select_state = false;
						SaveSaves();
						PlayWav(se_sys_click);
					}
				break;
			}
		}
/* up */if (controller&0x08 && !(lastkbState&0x08)) {
			switch (currentSelect) {
				case empty:
				case gamelist: currentSelect = menu;  PlayWav(se_sys_cursor); break;
				case saves: currentSelect = gamelist; PlayWav(se_sys_cursor); break;
				case display: displaySettingSelector = --displaySettingSelector<0?2:displaySettingSelector; PlayWav(se_sys_cursor); break;
				case options: optionSettingSelector = --optionSettingSelector<0?4:optionSettingSelector; PlayWav(se_sys_cursor); break;
				case about: if (scroll) { scroll--; PlayWav(se_sys_cursor);} break;
			}
		}
/*down*/if (controller&0x04 && !(lastkbState&0x04)) {
			switch (currentSelect) {
				case menu: 	
					currentSelect = courusel.size()?gamelist:empty; 
					PlayWav(se_sys_cursor); 
				break;
				case gamelist:
					if (select_to == stable_position) {
						if (courusel[sel]) {
							currentSelect = saves;
							no_select_state = false;
							if (select_save_state>=0 && select_save_state<=maxSaveState) {
								if (!courusel[sel]->savePointList[select_save_state]){
									no_select_state = true;
									for (uint8_t i = 0; i <= maxSaveState; i++) {
										if (courusel[sel]->savePointList[i]) {
											select_to_syspend_panel_selector = 6 + 8*i;
											select_save_state = i;
											no_select_state = false;
											break;
										}
									}
								} 
							}	
						}
					}
					PlayWav(se_sys_cursor);
				break;
				case display:
					displaySettingSelector = ++displaySettingSelector%3;
					PlayWav(se_sys_cursor);
				break;
				case options: 
					optionSettingSelector = ++optionSettingSelector%5;
					PlayWav(se_sys_cursor); break;
				case about: if (!scroll_end) { scroll++; PlayWav(se_sys_cursor); } break;
			}
		}
	/**********/
#ifdef DEBUG 
printf("DEBUG: %s.\n", "Clear tblName");
#endif

	memset(&tblName[0], 0xFF, TBL_SIZE);

#ifdef DEBUG 
printf("DEBUG: %s.\n", "Draw UI");
#endif

	if (currentSelect == display) {
		#ifdef DEBUG 
		printf("DEBUG: %s.\n", "Display Settings");
		#endif
		memset(&tblAttribute[0], 0x07, TBL_SIZE);
		drawDisplayPanel(counter%10);
		drawHintBar(currentSelect);
	} else if (currentSelect == options) {
		#ifdef DEBUG 
		printf("DEBUG: %s.\n", "System Options");
		#endif
		memset(&tblAttribute[0], 0x07, TBL_SIZE);
		drawOptionsPanel(counter%10);
		drawHintBar(currentSelect);
	} else if (currentSelect == about) {
		#ifdef DEBUG 
		printf("DEBUG: %s.\n", "Game Description");
		#endif
		memset(&tblAttribute[0], 0x07, TBL_SIZE);
		drawAboutPanel(counter%10);
		drawHintBar(currentSelect);
	} else if (currentSelect == manuals) {
		#ifdef DEBUG 
		printf("DEBUG: %s.\n", "Manuals");
		#endif
		memset(&tblAttribute[0], 0x07, TBL_SIZE);
		drawManualsPanel(counter%10);
		drawHintBar(currentSelect);
	} else {
		#ifdef DEBUG 
		printf("DEBUG: %s.\n", "other Draw");
		#endif
		memset(&tblAttribute[0], 0x05, TBL_SIZE);
		drawTopBar();
		drawMenu();
		if (currentSelect == saves) {
			drawTitleBar(4, 4, TBL_WIDTH-8);
			drawCourusel(7, counter%1);
			drawSavesPanel();
			#ifdef DEBUG 
			printf("DEBUG: %s.\n", "if tempsave drawTempSaveState");
			#endif
			if (temp_save_state && courusel[sel]->id == temp_save_state->id) drawTempSaveState(counter);
		} else {
			#ifdef DEBUG 
			printf("DEBUG: %s.\n", "game and prepare");
			#endif
			drawDownBar();
			drawTitleBar(4, 6, TBL_WIDTH-8);
			drawCourusel(9, counter%1);
			drawNavigate(counter%10);
			#ifdef DEBUG 
			printf("DEBUG: %s.\n", "drawTempSaveState");
			#endif
			drawTempSaveState(counter);
		}
		#ifdef DEBUG 
		printf("DEBUG: %s.\n", "drawSelector");
		#endif
		drawSelector(currentSelect);
		drawHintBar(currentSelect);
		drawHintWindow(counter%60);
	}

#ifdef DEBUG 
printf("DEBUG: %s.\n", "RenderFrameBuffer");
#endif

	renderFrameBuffer();
	if (currentSelect == preparegame) {
		if (fadeProcents<100) fadeProcents+=10;
		if (fadeProcents>100) fadeProcents=100;
	} else {
		if (fadeProcents>0) fadeProcents-=10;
		if (fadeProcents<0) fadeProcents=0;
	}

#ifdef DEBUG 
printf("DEBUG: %s.\n", "Update Complete");
#endif
}

void Shell::setJoyState(uint8_t kb) {
    #ifdef DEBUG
    printf("DEBUG: %s.\n", "Set Joy State");
    #endif
	lastkbState = controller;
    controller = kb;
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
	
	if (currentSelect != display && currentSelect != options && currentSelect != about && currentSelect != manuals) {
		for (uint16_t i = 0; i < game_count; i++) {
			uint16_t ndx = i%courusel.size();
			uint16_t x_additional = (88*courusel.size()) * (i/courusel.size());
			int16_t position_x = x_additional + courusel[ndx]->x_pos+courusel[ndx]->image->x;
			if (position_x < - 88) position_x += max_length_pix;
			else if (position_x > max_length_pix - 88) position_x -= max_length_pix;
			if (position_x <  0 - courusel[ndx]->image->width || position_x > SCREEN_SIZE) continue;
			for (int16_t y = courusel[ndx]->y_pos+courusel[ndx]->image->y; y < courusel[ndx]->y_pos+courusel[ndx]->image->y+courusel[ndx]->image->height; y++)
			for (int16_t x = position_x; x < position_x+courusel[ndx]->image->width; x++) {
				if (x >= SCREEN_SIZE) break;
				if (x < 0) continue;
				uint32_t pixel = (y - courusel[ndx]->y_pos-courusel[ndx]->image->y) * courusel[ndx]->image->width + (x - position_x); 
				txt_color = (courusel[ndx]->image->rawData[pixel].R << 16) | (courusel[ndx]->image->rawData[pixel].G << 8) | courusel[ndx]->image->rawData[pixel].B;
				PutPixel(x, y, txt_color);
			}
		}

		if (currentSelect == gamelist || currentSelect == menu || currentSelect == preparegame)
		for (uint8_t i = 0; i < courusel.size(); i++) {
			int16_t position_y = courusel[i]->navCard->y_pos;
			int16_t position_x = courusel[i]->navCard->x_pos + navigate_offset;
			if (position_x <  0 - courusel[i]->navCard->image->width || position_x > SCREEN_SIZE) continue;
			for (int16_t y = position_y; y < position_y + courusel[i]->navCard->image->height; y++)
			for (int16_t x = position_x; x < position_x + courusel[i]->navCard->image->width; x++) {
				if (x >= SCREEN_SIZE) break;
				if (x < 0) continue;
				uint32_t pixel = (y - position_y) * courusel[i]->navCard->image->width + (x - position_x);
				txt_color = (courusel[i]->navCard->image->rawData[pixel].R << 16) | (courusel[i]->navCard->image->rawData[pixel].G << 8) | courusel[i]->navCard->image->rawData[pixel].B;
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