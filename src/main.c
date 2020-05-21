#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include "./constants.h"
#include "./textures.h"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
int running = FALSE;
int ticksLastFrame;

uint32_t* colorBuffer = NULL;
SDL_Texture* colorBufferTexture;
//uint32_t* wallTexture = NULL; //For custom blue/black-grid texture

uint32_t* textures[NUM_TEX];

const int map[MAP_NUM_ROWS][MAP_NUM_COLS] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 ,1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 3, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 3, 0, 0, 0, 1},
    {1, 0, 7, 0, 7, 0, 7, 0, 5, 5, 5, 0, 4, 4, 0, 3, 0, 6, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 3, 0, 0, 0, 1},
    {1, 0, 7, 0, 7, 0, 7, 0, 0, 0, 0, 0, 0, 4, 0, 3, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 3, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

struct Player {
    float x;
    float y;
    float width;
    float height;
    int turnDirection; //-1 for left, +1 for right
    int walkDirection; //-1 for back, +1 for forward
    int strafeDirection;
    float rotationAngle;
    float walkSpeed;
    float turnSpeed;
} player;

struct Ray {
    float rayAngle;
    float wallHitX;
    float wallHitY;
    float distance;
    int wasHitVertical;

    //Helpers
    int isRayFacingUp;
    int isRayFacingDown;
    int isRayFacingLeft;
    int isRayFacingRight;
    int wallHitContent;
} rays[NUM_RAYS];

int wInit() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("Error initializing SDL.\n");
        return FALSE;
    }

    //No title, Xpos, wWidth, wHeight, [no-border] 
    window = SDL_CreateWindow(
        NULL,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_BORDERLESS
    );

    if (!window) {
        printf("Error creating SDL window.\n");
        return FALSE;
    }

    //use window, default driver, no flags
    renderer = SDL_CreateRenderer(
        window,
        -1,
        0
    );

    if (!renderer) {
        printf("Error creating SDL renderer.\n");
        return FALSE;
    }

    //texture mode
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    //If we make it this far, it would seem our window initialized correctly
    return TRUE;
}

void wDestroy() {
    //Release resources
    free(colorBuffer);
    //Destroy SDL in reverse order
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void setup() {
    player.x = WINDOW_WIDTH / 2;
    player.y = (WINDOW_HEIGHT / 2) + 300;
    player.width = 1;
    player.height = 1;
    player.turnDirection = 0;
    player.walkDirection = 0;
    player.strafeDirection = 0;
    player.rotationAngle = PI * 1.5;
    player.walkSpeed = 150;
    player.turnSpeed = 100 * (PI / 180);

    //allocate enough memory to hold the entire screen in the color buffer, cast everything
    colorBuffer = (uint32_t *) malloc(sizeof(uint32_t) * (uint32_t)WINDOW_WIDTH * (uint32_t)WINDOW_HEIGHT);

    //create an SDL texture to display the color buffer:
    colorBufferTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WINDOW_WIDTH,
        WINDOW_HEIGHT
    );

    //Create a texture with a pattern of blue & black lines
    //wallTexture = (uint32_t *)malloc(sizeof(uint32_t) * (uint32_t)TEX_WIDTH * (uint32_t)TEX_HEIGHT);
    //for (int x = 0; x < TEX_WIDTH; x++){
    //    for (int y = 0; y < TEX_HEIGHT; y++) {
    //        wallTexture[(TEX_WIDTH * y) + x] = ((x % 8) && (y % 8)) ? 0xFF0000FF : 0xFF000000;
    //    }
    //}

    //load textures from textures.h
    textures[0] = (uint32_t *)REDBRICK_TEXTURE;
    textures[1] = (uint32_t *)PURPLESTONE_TEXTURE;
    textures[2] = (uint32_t *)MOSSYSTONE_TEXTURE;
    textures[3] = (uint32_t *)GRAYSTONE_TEXTURE;
    textures[4] = (uint32_t *)COLORSTONE_TEXTURE;
    textures[5] = (uint32_t *)BLUESTONE_TEXTURE;
    textures[6] = (uint32_t *)WOOD_TEXTURE;
    textures[7] = (uint32_t *)EAGLE_TEXTURE;
}

int wallAt(float x, float y) {
    if (x < 0 || x > WINDOW_WIDTH || y < 0 || y > WINDOW_HEIGHT) {
        return TRUE;
    }

    int mapGridIndexX = floor(x / TILE_SIZE);
    int mapGridIndexY = floor(y / TILE_SIZE);
    return map[mapGridIndexY][mapGridIndexX] != 0;
}

void movePlayer(float deltaTime) {
    player.rotationAngle += player.turnDirection * player.turnSpeed * deltaTime;
    
    float moveStep = player.walkDirection * player.walkSpeed * deltaTime;
    float strafeStep = player.strafeDirection * player.walkSpeed * deltaTime;

    float newPlayerX;
    float newPlayerY;

    if (strafeStep && moveStep){
            newPlayerX = player.x + cos(player.rotationAngle) * moveStep;
            newPlayerX = newPlayerX + (cos(player.rotationAngle + (90 * PI/180))) * strafeStep;

            newPlayerY = player.y + sin(player.rotationAngle) * moveStep;
            newPlayerY = newPlayerY + (sin(player.rotationAngle + (90 * PI/180))) * strafeStep;

    } else {

        if (moveStep) {
            newPlayerX = player.x + cos(player.rotationAngle) * moveStep;
            newPlayerY = player.y + sin(player.rotationAngle) * moveStep;
        }

        if (strafeStep) {
            newPlayerX = player.x + (cos(player.rotationAngle + (90 * PI/180))) * strafeStep;
            newPlayerY = player.y + (sin(player.rotationAngle + (90 * PI/180))) * strafeStep;
        }
    }



    if (!wallAt(newPlayerX, newPlayerY)) {
        player.x = newPlayerX;
        player.y = newPlayerY;
    }
}

void renderPlayer() {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    SDL_Rect playerRect = {
        player.x * MINIMAP_SCALE_FACTOR,
        player.y * MINIMAP_SCALE_FACTOR,
        player.width * MINIMAP_SCALE_FACTOR,
        player.height * MINIMAP_SCALE_FACTOR
    };
    SDL_RenderFillRect(renderer, &playerRect);
    
    SDL_RenderDrawLine(
    renderer,
    MINIMAP_SCALE_FACTOR * player.x,
    MINIMAP_SCALE_FACTOR * player.y,
    MINIMAP_SCALE_FACTOR * player.x + cos(player.rotationAngle) * 40,
    MINIMAP_SCALE_FACTOR * player.y + sin(player.rotationAngle) * 40
);
}

float normalizeAngle(float angle) {
    angle = remainder(angle, TWO_PI);
    if (angle < 0) {
        angle = TWO_PI + angle;
    }
    return angle;
}

float distanceBetweenPoints(float x1, float y1, float x2, float y2) {
    return sqrt((x2-x1) * (x2-x1) + (y2-y1) *(y2-y1));
}

void castRay(float rayAngle, int stripId) {
    rayAngle = normalizeAngle(rayAngle);

    int isRayFacingDown = (rayAngle > 0) && (rayAngle < PI);
    int isRayFacingUp = !isRayFacingDown;

    int isRayFacingRight = (rayAngle < 0.5 * PI) || (rayAngle > 1.5 * PI);
    int isRayFacingLeft = !isRayFacingRight ;    
    
    float xintercept, yintercept;
    float xstep, ystep;

    ///////////////////////////////////
    //Horizontal Ray-Grid Intersection
    ///////////////////////////////////
    /*
    * Find coordinate of the first horizontal intersection (point A)
    * Find ystep
    * Find xstep
    * Convert intersection point (x,y) into map index [i,j]
    * if (intersection hits a wall): 
    *      store horizontal hit distance
    * else:
    *      find next horizontal intersection
    */

    int foundHorizontalWallHit = FALSE;
    float horizontalWallHitX = 0;
    float horizontalWallHitY = 0;
    int horizontalWallContent = 0;

    //Find the Y-Coordinate of the closest horizontal grid intersection
    yintercept = floor(player.y / TILE_SIZE) * TILE_SIZE;
    //If we're pointing down, we need to add another tile
    yintercept += isRayFacingDown ? TILE_SIZE : 0;
    
    //Find the X-Coordinate of the closest horizontal grid intersection
    xintercept = player.x + (yintercept - player.y) / tan(rayAngle);

    //Calculate the increment for xstep & ystep
    ystep = TILE_SIZE;
    ystep *= isRayFacingUp ? -1 : 1;

    xstep = TILE_SIZE / tan(rayAngle);
    xstep *= (isRayFacingLeft && xstep > 0) ? -1 : 1;
    xstep *= (isRayFacingRight && xstep < 0) ? -1 : 1;

    float nextHorizontalTouchX = xintercept;
    float nextHorizontalTouchY = yintercept;

    //Increment xstep and ystep until we find a wall
    while (nextHorizontalTouchX >= 0 && nextHorizontalTouchX <= WINDOW_WIDTH && nextHorizontalTouchY >= 0 && nextHorizontalTouchY <= WINDOW_HEIGHT) {

        float xCheck = nextHorizontalTouchX;
        float yCheck = nextHorizontalTouchY + (isRayFacingUp ? -1 : 0);

        if (wallAt(xCheck, yCheck)) {
            //found a wall hit
            horizontalWallHitX = nextHorizontalTouchX;
            horizontalWallHitY = nextHorizontalTouchY;
            horizontalWallContent = map[(int)floor(yCheck / TILE_SIZE)][(int)floor(xCheck / TILE_SIZE)];
            foundHorizontalWallHit = TRUE;
            break;

        } else {
            nextHorizontalTouchX += xstep;
            nextHorizontalTouchY += ystep;
        }

    }

    ///////////////////////////////////
    //Vertical Ray-Grid Intersection
    ///////////////////////////////////
    /**
     * Find coordinate of the first vertical intersection 
     * Find xstep
     * Find ystep
     * Convert intersection point (x,y) into map index [i,j]
     * if (intersection hits a wall): 
     *      store vertical hit distance
     * else:
     *      find next vertical intersection
     */

    int foundVerticalWallHit = FALSE;
    float verticalWallHitX = 0;
    float verticalWallHitY = 0;
    int verticalWallContent = 0;

    //Find the X-Coordinate of the closest horizontal grid intersection
    xintercept = floor(player.x / TILE_SIZE) * TILE_SIZE;
    //If we're pointing down, we need to add another tile
    xintercept += isRayFacingRight ? TILE_SIZE : 0;
    
    //Find the Y-Coordinate of the closest horizontal grid intersection
    yintercept = player.y + (xintercept - player.x) * tan(rayAngle);

    //Calculate the increment for xstep & ystep
    xstep = TILE_SIZE;
    xstep *= isRayFacingLeft ? -1 : 1;

    ystep = TILE_SIZE * tan(rayAngle);
    ystep *= (isRayFacingUp && ystep > 0) ? -1 : 1;
    ystep *= (isRayFacingDown && ystep < 0) ? -1 : 1;

    float nextVerticalTouchX = xintercept;
    float nextVerticalTouchY = yintercept;

    //Increment xstep and ystep until we find a wall
    while (nextVerticalTouchX >= 0 && nextVerticalTouchX <= WINDOW_WIDTH && nextVerticalTouchY >= 0 && nextVerticalTouchY <= WINDOW_HEIGHT) {

        float xCheck = nextVerticalTouchX + (isRayFacingLeft ? -1 : 0);
        float yCheck = nextVerticalTouchY;

        if (wallAt(xCheck, yCheck)) {
            //found a wall hit
            verticalWallHitX = nextVerticalTouchX;
            verticalWallHitY = nextVerticalTouchY;
            verticalWallContent = map[(int)floor(yCheck / TILE_SIZE)][(int)floor(xCheck / TILE_SIZE)];
            foundVerticalWallHit = TRUE;
            break;

        } else {
            nextVerticalTouchX += xstep;
            nextVerticalTouchY += ystep;
        }

    }

    /////////////////////////////////////////////////////////////////////////////
    //Calculate both horizontal & vertical distances => choose the smallest value
    float horizontalHitDistance = (foundHorizontalWallHit) 
        ? distanceBetweenPoints(player.x, player.y, horizontalWallHitX, horizontalWallHitY)
        : INT_MAX;

    float verticalHitDistance = (foundVerticalWallHit)
        ? distanceBetweenPoints(player.x, player.y, verticalWallHitX, verticalWallHitY)
        : INT_MAX;

    //Only store the shortest distance
    if (verticalHitDistance < horizontalHitDistance) {
        rays[stripId].wallHitX = verticalWallHitX;
        rays[stripId].wallHitY = verticalWallHitY;
        rays[stripId].distance = verticalHitDistance;
        rays[stripId].wallHitContent = verticalWallContent;
        rays[stripId].wasHitVertical = TRUE;
    } else {
        rays[stripId].wallHitX = horizontalWallHitX;
        rays[stripId].wallHitY = horizontalWallHitY;
        rays[stripId].distance = horizontalHitDistance;
        rays[stripId].wallHitContent = horizontalWallContent;
        rays[stripId].wasHitVertical = FALSE;
    }
    
    rays[stripId].rayAngle = rayAngle;
    rays[stripId].isRayFacingDown = isRayFacingDown;
    rays[stripId].isRayFacingUp = isRayFacingUp;
    rays[stripId].isRayFacingLeft = isRayFacingLeft;
    rays[stripId].isRayFacingRight = isRayFacingRight;
}

void castAllRays() {
    //Get the leftmost ray => subtract half FOV
    float rayAngle = player.rotationAngle - (FOV / 2);

    for (int stripId = 0; stripId < NUM_RAYS; stripId++) {
        castRay(rayAngle, stripId);
        rayAngle += (FOV / NUM_RAYS);
    }
}

void renderMap() {
    for (int i = 0; i < MAP_NUM_ROWS; i++) {
        for (int j = 0; j < MAP_NUM_COLS; j++) {
            int tileX = j * TILE_SIZE;
            int tileY = i * TILE_SIZE;
            int tileColor = map[i][j] != 0 ? 255 : 0;

            SDL_SetRenderDrawColor(renderer, tileColor, tileColor, tileColor, 255);
            SDL_Rect mapTileBox = {
                tileX * MINIMAP_SCALE_FACTOR,
                tileY * MINIMAP_SCALE_FACTOR,
                TILE_SIZE * MINIMAP_SCALE_FACTOR,
                TILE_SIZE * MINIMAP_SCALE_FACTOR
            };
            SDL_RenderFillRect(renderer, &mapTileBox);
        }
    }
}

void renderRays() {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    for (int i = 0; i < NUM_RAYS; i++) {
        SDL_RenderDrawLine(
            renderer,
            player.x * MINIMAP_SCALE_FACTOR,
            player.y * MINIMAP_SCALE_FACTOR,
            rays[i].wallHitX * MINIMAP_SCALE_FACTOR,
            rays[i].wallHitY * MINIMAP_SCALE_FACTOR
        ); 
    }
}

void processInput() {
    SDL_Event event;

    do {
        switch(event.type) {
            case SDL_QUIT: {
                running = FALSE;
                break;
            }

            case SDL_KEYDOWN: {
                if (event.key.keysym.sym == SDLK_ESCAPE) 
                    running = FALSE;

                if (event.key.keysym.sym == SDLK_UP) 
                    player.walkDirection = 1;
                if (event.key.keysym.sym == SDLK_w) 
                    player.walkDirection = 1;
                if (event.key.keysym.sym == SDLK_DOWN) 
                    player.walkDirection = -1;
                if (event.key.keysym.sym == SDLK_s) 
                    player.walkDirection = -1;
                if (event.key.keysym.sym == SDLK_RIGHT) 
                    player.turnDirection = 1;
                if (event.key.keysym.sym == SDLK_d) 
                    player.turnDirection = 1;
                if (event.key.keysym.sym == SDLK_LEFT) 
                    player.turnDirection = -1;
                if (event.key.keysym.sym == SDLK_a) 
                    player.turnDirection = -1;

                if (event.key.keysym.sym == SDLK_e)
                    player.strafeDirection = 1;
                if (event.key.keysym.sym == SDLK_q) 
                    player.strafeDirection = -1;

                break;
            }

            case SDL_KEYUP: {
                if (event.key.keysym.sym == SDLK_UP)
                    player.walkDirection = 0;
                if (event.key.keysym.sym == SDLK_w)
                    player.walkDirection = 0;
                if (event.key.keysym.sym == SDLK_DOWN)
                    player.walkDirection = 0;
                if (event.key.keysym.sym == SDLK_s)
                    player.walkDirection = 0;
                if (event.key.keysym.sym == SDLK_RIGHT)
                    player.turnDirection = 0;
                if (event.key.keysym.sym == SDLK_d)
                    player.turnDirection = 0;
                if (event.key.keysym.sym == SDLK_LEFT)
                    player.turnDirection = 0;
                if (event.key.keysym.sym == SDLK_a)
                    player.turnDirection = 0;
                    
                if (event.key.keysym.sym == SDLK_e)
                    player.strafeDirection = 0;
                if (event.key.keysym.sym == SDLK_q)
                    player.strafeDirection = 0;
            
                break;
            }
        }

    } while (SDL_PollEvent(&event));

}

void update() {
    //Waste some time until we reach the target frametime:
    //Naive solution:
    //while (!SDL_TICKS_PASSED(SDL_GetTicks(), ticksLastFrame + FRAME_TIME));
    
    //Better solution:
    //Compute how long until we reach the target frametime in ms
    int wait = FRAME_TIME - (SDL_GetTicks() - ticksLastFrame);
    //Only delay if we're going too fast
    if (wait > 0 && wait <= FRAME_TIME) {
        SDL_Delay(wait);
    }

    //Decouple object movement from framerate:
    //Delta time: Pixels per frame => pixels per second
    //Difference in ticks from last frame converted to seconds
    float deltaTime = (SDL_GetTicks() - ticksLastFrame) / 1000.0f;
    ticksLastFrame = SDL_GetTicks();

    movePlayer(deltaTime);
    castAllRays();
}

void generateProjection() {
    for (int i = 0; i < NUM_RAYS; i++) {
        //remove fishbowl distortion
        float correctedDistance = rays[i].distance * cos(rays[i].rayAngle - player.rotationAngle);

        float distanceProjPlane = (WINDOW_WIDTH / 2) / tan(FOV / 2);
        float projectedWallHeight = (TILE_SIZE / correctedDistance) * distanceProjPlane;

        int wallStripHeight = (int)projectedWallHeight;

        int wallTopPixel = (WINDOW_HEIGHT / 2) - (wallStripHeight / 2);
        wallTopPixel = wallTopPixel < 0 ? 0 : wallTopPixel;

        int wallBottomPixel = (WINDOW_HEIGHT / 2) + (wallStripHeight / 2);
        wallBottomPixel = wallBottomPixel > WINDOW_HEIGHT ? WINDOW_HEIGHT : wallBottomPixel;

        //ceiling
        for (int y = 0; y < wallTopPixel; y++) {
            colorBuffer[(WINDOW_WIDTH * y) + i] = 0xFF333333;
        }

        //Calculate texture offset X
        int textureOffsetX;
        if (rays[i].wasHitVertical) {
            //perform offset for vertical hit
            textureOffsetX = (int)rays[i].wallHitY % TILE_SIZE;
        } else {
            //perform offset for horizontal hit
            textureOffsetX = (int)rays[i].wallHitX % TILE_SIZE;
        }

        //walls + textures
        //get the correct texture id # from the map content
        int texNum = rays[i].wallHitContent - 1;

        for (int y = wallTopPixel; y < wallBottomPixel; y++) {
            int distanceTop = y + (wallStripHeight / 2) - (WINDOW_HEIGHT / 2);
            int textureOffsetY = distanceTop * ((float)TEX_HEIGHT / wallStripHeight);

            //set the color of the wall based on the color from the texture in memory
            uint32_t texturePixelColor = textures[texNum][(TEX_WIDTH * textureOffsetY) + textureOffsetX];
            colorBuffer[(WINDOW_WIDTH * y) + i] = texturePixelColor;
        }

        //floor
        for (int y = wallBottomPixel; y < WINDOW_HEIGHT; y++) {
            colorBuffer[(WINDOW_WIDTH * y) + i] = 0xFF777777;
        }
    }
}

void clearColorBuffer(uint32_t color) {
    for (int x = 0; x < WINDOW_WIDTH; x++) {
        for (int y = 0; y < WINDOW_HEIGHT; y++) {
            if (x == y) {
                colorBuffer[ (WINDOW_WIDTH * y) + x ] = color;
            } else {
                colorBuffer[ (WINDOW_WIDTH * y) + x ] = 0xFF000000;
            }
        }
    }
}

void renderColorBuffer() {
    SDL_UpdateTexture(
        colorBufferTexture,
        NULL, 
        colorBuffer,
        (int)(uint32_t)WINDOW_WIDTH * sizeof(uint32_t)
    );
    SDL_RenderCopy(renderer, colorBufferTexture, NULL, NULL);
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    generateProjection();

    renderColorBuffer();
    clearColorBuffer(0xFF000000);

    //Render minimap
    renderMap();
    //Render all rays for the current frame
    renderRays();
    //Render player for the current frame
    renderPlayer();

    //Swap back buffer to front
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    running = wInit();
    setup();

    while (running) {
        processInput();
        update();
        render();
    }

    wDestroy();
    return 0;
}
