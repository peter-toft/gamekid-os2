// library.c
// Gamekid by Dustin Mierau

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "libraryview.h"
#include "pd_api.h"
#include "list.h"
#include "app.h"

typedef struct _GKLibraryView {
	GKApp* app;
	int selection;
	List* list;
	int list_length;
	int scroll_y;
	LCDBitmap* icon_image;
	LCDFont* font;
} GKLibraryView;


GKLibraryView* GKLibraryViewCreate(GKApp* app) {
	GKLibraryView* view = malloc(sizeof(GKLibraryView));
	memset(view, 0, sizeof(GKLibraryView));
	view->app = app;
	view->selection = 0;
	view->list = ListCreate();
	view->list_length = 0;
	view->scroll_y = 0;
	view->icon_image = playdate->graphics->loadBitmap("images/browser-icon", NULL);
	view->font = playdate->graphics->loadFont("fonts/Asheville-Sans-14-Bold", NULL);
	return view;
}

void GKLibraryViewDestroy(GKLibraryView* view) {
	if(view->list != NULL) {
		ListDestroy(view->list);
	}
	if(view->icon_image != NULL) {
		playdate->graphics->freeBitmap(view->icon_image);
	}
	if(view->font != NULL) {
		free(view->font);
	}
	free(view);
}

#pragma mark -

void listFilesCallback(const char* filename, void* context) {
	GKLibraryView* view = (GKLibraryView*)context;
	
	if(filename[0] == '.') {
		return;
	}
	
	int filename_length = strlen(filename);
	if(filename_length < 3 || filename[filename_length-3] != '.' || filename[filename_length-2] != 'g' || filename[filename_length-1] != 'b') {
		return;
	}
	
	void* filename_data = malloc(strlen(filename) + 1);
	memset(filename_data, 0, strlen(filename) + 1);
	strcpy(filename_data, filename);
	ListAppend(view->list, filename_data);
	view->list_length++;
}

void GKLibraryViewShow(GKLibraryView* view) {
	ListDestroyAll(view->list);
	view->list = ListCreate();
	view->list_length = 0;
	view->scroll_y = 0;
	view->selection = 0;
	
	playdate->file->listfiles("/games", listFilesCallback, view);
}

#define LIST_ROW_HEIGHT 36
#define LIST_X 0
#define LIST_HEIGHT 240
#define LIST_WIDTH (400)

void GKLibraryDraw(GKLibraryView* view) {
	// playdate->graphics->fillRect(LIST_X, 0, LIST_WIDTH, LIST_HEIGHT, kColorWhite);
	playdate->graphics->clear(kColorWhite);
	playdate->graphics->setFont(view->font);
	
	if(view->list_length > 0) {
		ListNode* node = view->list->first;
		int i = 0;
		while(node->next != NULL) {
			if(i == view->selection) {
				playdate->graphics->fillRect(LIST_X, -view->scroll_y + LIST_ROW_HEIGHT * i, LIST_WIDTH, LIST_ROW_HEIGHT, kColorBlack);
				playdate->graphics->setDrawMode(kDrawModeFillWhite);
			}
			else {
				playdate->graphics->setDrawMode(kDrawModeFillBlack);
			}
			
			playdate->graphics->drawBitmap(view->icon_image, LIST_X + 10, -view->scroll_y + 10 + LIST_ROW_HEIGHT * i, kBitmapUnflipped);
			playdate->graphics->drawText(node->data, strlen((char*)node->data), kASCIIEncoding, LIST_X + 10 + 23, -view->scroll_y + 10 + LIST_ROW_HEIGHT * i);
			
			i++;
			node = node->next;
		}
	}
	else {
		const char* message = "Place your .gb files in\nGamekid's games folder\non your Playdate's Data Disk.\n\nThis is still very beta, enjoy!";
		playdate->graphics->setDrawMode(kDrawModeFillBlack);
		playdate->graphics->drawText(message, strlen(message), kASCIIEncoding, 30, 30);
	}

	playdate->graphics->setDrawMode(kDrawModeCopy);
}

void GKLibraryViewUpdate(GKLibraryView* view, unsigned int dt) {
	PDButtons buttons_pushed;
	playdate->system->getButtonState(NULL, &buttons_pushed, NULL);
	
	if(buttons_pushed & kButtonDown) {
		view->selection = GKMin(view->selection+1, view->list_length-1);
	}
	
	if(buttons_pushed & kButtonUp) {
		view->selection = GKMax(view->selection-1, 0);
	}
	
	if((buttons_pushed & kButtonA) && (view->list_length > 0)) {
		void* filename = ListGet(view->list, view->selection);
		char path[256];
		
		memset(path, 0, 256);
		strcpy(path, "games/");
		strcpy(path + 6, filename);
		
		GKAppGoToGame(view->app, path);
	}
	
	// Update scroll.
	int scroll_bottom = view->scroll_y + LIST_HEIGHT;
	int line_y = 0;
	
	ListNode* node = view->list->first;
	int i = 0;
	while(node->next != NULL) {
		
		if(i == view->selection) {
			int line_height = LIST_ROW_HEIGHT;
			int line_bottom = line_y + line_height;
			
			if(view->scroll_y > line_y) {
				view->scroll_y = line_y - 0;
			}
			else if(scroll_bottom < line_bottom) {
				view->scroll_y += (line_bottom - scroll_bottom);
			}
		}
		
		line_y += LIST_ROW_HEIGHT + 0;
		node = node->next;
		i++;
	}

	GKLibraryDraw(view);
}