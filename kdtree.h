#include <conio.h>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <time.h>
#include <vector>
#include <algorithm>
using namespace std;

// A few notes...
// Make sure all points are unique before inserting them into the Kdtree.
// The only functions you should call are the constructor and searchtree:
// Kdtree(const vector<Point> &P)
// vector<Point> searchtree(float x0, float y0, float x1, float y1)

struct Point {
    float x, y;
    Segment seg;   
    
    bool operator==(const Point &p2)
    {
        return (x == p2.x && y == p2.y);
    }
    
    Point(float x, float y, Segment seg)
    {
        this->x = x;
        this->y = y;
        this->seg = seg;
    }
    
    
    Point()
    {
    }
    
};


class Kdtree {
    
    struct Node
    {
        Point p;       
        Node *left, *right;       
        
        Node(const Point &p)
        {
            this->p = p;           
            this->left = NULL;
            this->right = NULL;
        }
        
    };
    
    Node *root;    
    
    bool lessthanx(const Point &p1, const Point &p2)
    {
        if (p1.x != p2.x) return (p1.x < p2.x);
        else return (p1.y < p2.y);
    }
    
    bool lessthany(Point &p1, Point &p2)
    {
        if (p1.y != p2.y) return (p1.y < p2.y);
        else return (p1.x < p2.x);
    }
    
    struct PointSortX
    {
        bool operator () (const Point& p1, const Point &p2)
        {
            
            if (p1.x != p2.x) return (p1.x < p2.x);
            else return (p1.y < p2.y);                        
        }
    };
    
    struct PointSortY
    {
        bool operator () (const Point& p1, const Point &p2)
        {
            if (p1.y != p2.y) return (p1.y < p2.y);
            else return (p1.x < p2.x);
        }
    };
    
    
    struct Range {
        float x0, y0;
        float x1, y1;       
        
        Range(float x0, float y0, float x1, float y1)
        {
            this->x0 = x0;
            this->y0 = y0;
            this->x1 = x1;
            this->y1 = y1;
        }
        
        Range(void)
        {
        }
        
    };
    
    // depth is 0 for an x level
    // depth is 1 for a y level
    
    Node* buildkdtree(vector<Point> &xsorted, vector<Point> &ysorted, int depth)
    {
        
        if (xsorted.size()==0) return NULL; 
        else if (xsorted.size()==1) return new Node(xsorted[0]);     
        
        int n = xsorted.size();
        int medianindex = (n-1)/2;
        
        if (!depth) {
            Point L = xsorted[medianindex];
            vector<Point> P1newxsorted(xsorted.begin(), xsorted.begin()+medianindex+1);
            vector<Point> P2newxsorted(xsorted.begin()+medianindex+1, xsorted.end());
            vector<Point> P1newysorted, P2newysorted;
            vector<Point>::iterator i = ysorted.begin();
            while (i != ysorted.end())
            {
                Point p = *i;
                if (lessthanx(p, L) || p == L) P1newysorted.push_back(p);
                else P2newysorted.push_back(p);
                ++i;
            }
            
            Node *v = new Node(L);
            v->left = buildkdtree(P1newxsorted, P1newysorted, !depth);
            v->right = buildkdtree(P2newxsorted, P2newysorted, !depth);
            return v;     
        }
        
        else {
            Point L = ysorted[medianindex];
            
            vector<Point> P1newysorted(ysorted.begin(), ysorted.begin()+medianindex+1);
            vector<Point> P2newysorted(ysorted.begin()+medianindex+1, ysorted.end());
            vector<Point> P1newxsorted, P2newxsorted;
            
            vector<Point>::iterator i = xsorted.begin();
            while (i != xsorted.end())
            {
                Point p = *i++;
                if (lessthany(p, L) || p == L) P1newxsorted.push_back(p);
                else P2newxsorted.push_back(p);
            }
            Node *v = new Node(L);
            v->left = buildkdtree(P1newxsorted, P1newysorted, !depth);
            v->right = buildkdtree(P2newxsorted, P2newysorted, !depth);
            return v;     
        }
    }
    
    
    
    Node* buildkdtreemain(const vector<Point> &P)
    {
        if (P.size()==0) return NULL; 
        else if (P.size()==1) return new Node(P[0]);     
        vector<Point> xsorted(P);
        sort(xsorted.begin(), xsorted.end(), PointSortX());
        vector<Point> ysorted(P);
        sort(ysorted.begin(), ysorted.end(), PointSortY());
        return buildkdtree(xsorted, ysorted, 0);
    }
    
    
    bool pointinregion(const Point &p, const Range &r)
    {
        return (
        p.x >= r.x0
        && p.x <= r.x1
        && p.y >= r.y0
        && p.y <= r.y1);
    }
    
    bool isfullycontained(const Range &small, const Range &big)
    {
        return (small.x0 >= big.x0
        && small.y0 >= big.y0
        && small.x1 <= big.x1
        && small.y1 <= big.y1);        
    }
    
    bool intersects(const Range &r0, const Range &r1)
    {
        if (r0.x0 > r1.x1) return false;
        else if (r1.x0 > r0.x1) return false;
        else if (r0.y0 > r1.y1) return false;
        else if (r1.y0 > r0.y1) return false;
        else return true;
    }
    
    void reportsubtree(Node *v, vector<Point> &pointoutputvector)
    {
        if (v == NULL) return;
        reportsubtree(v->left, pointoutputvector);
        reportsubtree(v->right, pointoutputvector);
        
        if (!v->right && !v->left) pointoutputvector.push_back(v->p);
    }
    
    
    // depth will be 0 if we're at an x partition and 1 if we're at a y partition
    void searchkdtree(Node *v, const Range &R, int depth, const Range &regionv, vector<Point> &pointoutputvector)
    {
        if (v == NULL) return;
        //       cout << "yo" << endl; 
        if (!v->left && !v->right)
        {
            if (pointinregion(v->p, R)) pointoutputvector.push_back(v->p);
            return;
        }
        
        
        Range leftregion = regionv;
        if (!depth) leftregion.x1 = v->p.x;
        else leftregion.y1 = v->p.y;
        
        if (isfullycontained(leftregion, R)) reportsubtree(v->left, pointoutputvector);
        else if (intersects(leftregion, R)) searchkdtree(v->left, R, !depth, leftregion, pointoutputvector);
        
        Range rightregion = regionv;
        if (!depth) rightregion.x0 = v->p.x;
        else rightregion.y0 = v->p.y;
        
        if (isfullycontained(rightregion, R)) reportsubtree(v->right, pointoutputvector);
        else if (intersects(rightregion, R)) searchkdtree(v->right, R, !depth, rightregion, pointoutputvector);
        
    }
    
    
    
    void searchkdtree(Node *v, Range R, vector<Point> &pointoutputvector)
    {
        
        if (v == NULL) return;
        
        Range regionv;
        float infinity = std::numeric_limits< float >::infinity();
        
        regionv.x0 = -infinity;
        regionv.y0 = -infinity;
        regionv.x1 = infinity;
        regionv.y1 = infinity;
        
        searchkdtree(v, R, 0, regionv, pointoutputvector);
        
    }
    
    
    void deleteallnodes(Node *root)
    {
        if (root == NULL) return;
        deleteallnodes(root->left);
        deleteallnodes(root->right);
        delete root;
    }
    
    public:
    Kdtree(const vector<Point> &P)
    {
        root = buildkdtreemain(P);
    }
    
    vector<Point> searchtree(float x0, float y0, float x1, float y1)
    {
        vector<Point> pointoutputvector;
        searchkdtree(root, Range(x0, y0, x1, y1), pointoutputvector);
        return pointoutputvector;
    }
    
    void searchtree(float x0, float y0, float x1, float y1, vector<Point> &pointoutputvector)
    {
        searchkdtree(root, Range(x0, y0, x1, y1), pointoutputvector);
    }
    
    
    
    ~Kdtree()
    {
        deleteallnodes(root);
    }
    
};


