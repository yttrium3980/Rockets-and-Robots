#include <vector>

class Maze
{

class Box
{
    unsigned left:1;
    unsigned right:1;
    unsigned top:1;
    unsigned bottom:1;
    
    public:
    
    Box()
    {
        left=1;
        right=1;
        top=1;
        bottom=1;
    }
    
    void settop(int value)
    {
        top = value;
    }
    
    void setbottom(int value)
    {
        bottom = value;
    }
    
    void setleft(int value)
    {
        left = value;
    }
    
    void setright(int value)
    {
        right = value;
    }
    
    int getright()
    {
        return right;
    }
    
    int getleft()
    {
        return left;
    }
    
    int gettop()
    {
        return top;
    }
    
    int getbottom()
    {
        return bottom;
    }
    
};

    Box *mazeboxes;
    int width, height;
    bool *visited;
    bool *blockmaze;       
    
    bool canmove(int x, int y)
    {
        return (canmoveup(x, y) || canmovedown(x, y) ||
        canmoveleft(x, y) || canmoveright(x, y));
    }
    
    bool canmoveleft(int x, int y)
    {
        return (x > 0 && !visited[y*width+x-1]);
    }
    
    bool canmoveright(int x, int y)
    {
        return (x < (width-1) && !visited[y*width+x+1]);
    }
    
    bool canmoveup(int x, int y)
    {
        return (y > 0 && !visited[(y-1)*width+x]);
    }
    
    bool canmovedown(int x, int y)
    {
        return (y < (height-1) && !visited[(y+1)*width+x]);
    }
    
    
    void visit(int x, int y)
    {
        visited[y*width + x] = true;
        
        Box &b = mazeboxes[y*width + x];
        
        while (canmove(x, y))
        {            
            int movement = rand()%4;
            
/*static int lastmove = 99;
static int numinarow = 0;
if (lastmove == movement) numinarow++;
else numinarow=0;
if (numinarow>=3) {numinarow=0; return;}
lastmove = movement;*/


            switch(movement)
            {
                
                case 0:
                if (canmoveright(x, y)) 
                {
                    b.setright(0);
                    Box &rightbox = mazeboxes[y*width + x+1];
                    rightbox.setleft(0);
                    visit(x+1, y);
                }
                break;
                
                
                case 1:
                if (canmoveleft(x, y)) 
                {
                    b.setleft(0);
                    Box &leftbox = mazeboxes[y*width + x-1];
                    leftbox.setright(0);
                    visit(x-1, y);
                }
                break;
                
                
                case 2:
                if (canmovedown(x, y)) 
                {
                    b.setbottom(0);
                    Box &bottombox = mazeboxes[(y+1)*width + x];
                    bottombox.settop(0);
                    visit(x, y+1);
                }
                break;
                
                
                case 3:
                if (canmoveup(x, y)) 
                {
                    b.settop(0);
                    Box &topbox = mazeboxes[(y-1)*width + x];
                    topbox.setbottom(0);
                    visit(x, y-1);
                }
                break;                                
            }            
        }                
    }
    
    void initializeblockmaze(void)
    {
        
        blockmaze = new bool[width*3*height*3];
        memset(blockmaze, 0, sizeof(bool)*width*3*height*3);
        for (int x = 0; x < width; x++)
        for (int y = 0; y < height; y++)
        {
            Box &b = mazeboxes[y*width+x];
            if (b.gettop()) 
            {
                blockmaze [y*3*width*3+x*3] = true;
                blockmaze [y*3*width*3+x*3+1] = true;
                blockmaze [y*3*width*3+x*3+2] = true;
            }
            
            if (b.getbottom()) 
            {
                blockmaze [(y*3+2)*width*3+x*3] = true;
                blockmaze [(y*3+2)*width*3+x*3+1] = true;
                blockmaze [(y*3+2)*width*3+x*3+2] = true;
            }
            
            if (b.getleft()) 
            {
                blockmaze [(y*3+0)*width*3+x*3] = true;
                blockmaze [(y*3+1)*width*3+x*3] = true;
                blockmaze [(y*3+2)*width*3+x*3] = true;
            }
            
            if (b.getright()) 
            {
                blockmaze [(y*3+0)*width*3+x*3+2] = true;
                blockmaze [(y*3+1)*width*3+x*3+2] = true;
                blockmaze [(y*3+2)*width*3+x*3+2] = true;
            }
            
            
        }
        
        
    }
    
    public:
    
    Maze(int width, int height)
    {
        this->width = width;
        this->height = height;
        mazeboxes = new Box[width*height];
        
        visited = new bool[width*height];
        for (int ctr = 0; ctr < width*height; ctr++) visited[ctr] = false;
        
        // initialize the maze
        visit(0, 0);
        
		//visit(width/2, height/2);
        
        initializeblockmaze();
        
        delete[] visited;        
    }
    
    ~Maze()
    {
        delete [] blockmaze;
        delete[] mazeboxes;       
    }
    
    void draw(void)
    {
        for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
        {
            Box b = mazeboxes[y*width+x];
            
            float x0 = float(x)/width;
            float y0 = float(y)/height;
            
            float x1 = float(x+1)/width;
            float y1 = float(y+1)/height;
            
            glBegin(GL_LINES);
        if (b.gettop()) {glVertex2f(x0, y0); glVertex2f(x1, y0);}
        if (b.getbottom()) {glVertex2f(x0, y1); glVertex2f(x1, y1);}            
        if (b.getleft()) {glVertex2f(x0, y0); glVertex2f(x0, y1);}
        if (b.getright()) {glVertex2f(x1, y0); glVertex2f(x1, y1);}                        
            glEnd();
            
        }
        
    }
    
    vector<Segment> getsegments(void)
    {
        
        vector<Segment> segments;                
        
        for (int y = 0; y < height*3; y++)
        for (int x = 0; x < width*3; x++)
        {
            bool iswall = blockmaze[y*(width*3) + x];
            if (!iswall) continue;
            
            float x0 = float(x)/(width*3);
            float y0 = float(y)/(height*3);
            
            float x1 = float(x+1)/(width*3);
            float y1 = float(y+1)/(height*3);
            
            // top   
            if (y > 0 && !blockmaze[(y-1)*(width*3) + x])                                 
            //segments.push_back(Segment(x0, y0, x1, y0));
            segments.push_back(Segment(x1, y0, x0, y0));

            // bottom
            if (y < height*3-1 && !blockmaze[(y+1)*(width*3) + x])                                 
            segments.push_back(Segment(x0, y1, x1, y1));
            
            // left       
            if (x > 0 && !blockmaze[y*(width*3) + x-1])                                    
            segments.push_back(Segment(x0, y0, x0, y1));
            
            // right    
            if (x < width*3-1 && !blockmaze[y*(width*3) + x+1])                              
            //segments.push_back(Segment(x1, y0, x1, y1));
            segments.push_back(Segment(x1, y1, x1, y0));
            
        }                
        return segments;                
    }
    
    void drawsegments(void)
    {
        vector<Segment> s = getsegments();
        
        vector<Segment>::iterator i = s.begin();
        
        glBegin(GL_LINES);
        while (i != s.end())
        {
            Segment &seg = *i;      
            glVertex2f(seg.x0, seg.y0);      
            glVertex2f(seg.x1, seg.y1);      
            ++i;      
        }     
        glEnd();
        
    }
    
    
    void drawblockmaze(void)
    {
        for (int y = 0; y < height*3; y++)
        for (int x = 0; x < width*3; x++)
        {
            bool iswall = blockmaze[y*(width*3) + x];
            
            float x0 = float(x)/(width*3);
            float y0 = float(y)/(height*3);
            
            float x1 = float(x+1)/(width*3);
            float y1 = float(y+1)/(height*3);
            
            if (iswall) {
                glBegin(GL_QUADS);
                glVertex2f(x0, y0);
                glVertex2f(x1, y0);
                glVertex2f(x1, y1);
                glVertex2f(x0, y1);
                glEnd();
            }
            
            
            
        }
        
    }

    // returns whether there is a wall or not at the x and y coords    
    bool isblockwall(float x, float y)
    {
        int index = int(int(y*height*3)*(width*3)) + int(x*(width*3));
        return blockmaze[index];                        
    }
    
    
};
