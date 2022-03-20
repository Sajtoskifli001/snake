#include "graphics.hpp"
#include <vector>
#include <fstream>
#include <sstream>

using namespace genv;
using namespace std;

const int w=700,h=700,  //v�szon sz�less�ge, magass�ga (pixelben), v�ltoztathat�
          blocksize=20, //r�cs n�gyzet m�rete (pixelben) - a program 20 k�r� �p�lt, ennek megv�ltoztat�sa vil�gkatasztr�f�t eredm�nyez (�s m�g a program sem fog m�k�dni)
          falvastag=2,  //p�lyasz�li oldalfal vastags�ga (blockban), v�ltoztathat�
          beltav=4;     //belso fal oldalfalt�l val� t�vols�ga (blockban), v�ltoztathat�

struct pont             //koordin�ta mez�
{
    int x,y;
};

struct szin             //k�ppont mez�
{
    int r,g,b;
};

struct block            //r�cs egy blockja
{
    pont koord;
    char status='s';    //s=szabad, f=fal, t=test, (p=volt test), a=alma
};

bool operator==(const pont &a, const pont &b)   //k�t koordin�ta �sszehasonl�t�sa
{
    return (a.x==b.x and a.y==b.y);
}

struct Textpanel                                //sz�vegdoboz f�men�re
{
    protected:
        pont koord;
        szin txtcolor;
        int szel,mag;
        string szoveg;

    public:
    Textpanel(string s)
    {
        szel=w/3;
        mag=h/10;
        koord.x=w/2-szel/2;
        koord.y=h-(mag*3.5);
        szoveg=s;
        txtcolor.r=255;
        txtcolor.g=120;
        txtcolor.b=0;
    }

    virtual void rajzol()
    {
        gout.load_font("LiberationMono-Bold.ttf",w/20);
        gout << color(67,67,67) << move_to(koord.x,koord.y) << box(szel,mag)
             << color(100,100,100) << move_to(koord.x+5,koord.y+5) << box(szel-10,mag-10)

             << color(txtcolor.r,txtcolor.g,txtcolor.b)
             << move_to(koord.x+(szel/2)-(gout.twidth(szoveg)/2),koord.y+(mag/2)-((gout.cascent()+gout.cdescent())/2))
             << text(szoveg);
    }

    bool rajta(int cx, int cy)                                                      //kurzor rajta van-e
    {
        return (cx>koord.x and cx<koord.x+szel and cy>koord.y and cy<koord.y+mag);
    }

    void colorchange(szin becolor)
    {
        txtcolor=becolor;
    }
};

struct Title:public Textpanel           //c�mmez� f�men�re (�s game over-re)
{
    protected:
        string alcim;

    public:
    Title(string s, string als): Textpanel(s)
    {
        koord.x=w/7;
        koord.y=h/7;
        szel=w/7*5;
        mag=h/5;
        alcim=als;
        txtcolor.r=0;
        txtcolor.g=255;
        txtcolor.b=0;
    }

    void rajzol() override
    {
        gout.load_font("LiberationMono-Bold.ttf",w/14);
        gout << color(100,0,0) << move_to(koord.x,koord.y) << box(szel,mag)
             << color(160,0,0) << move_to(koord.x+10,koord.y+10) << box(szel-20,mag-20)                             //keret

             << color(txtcolor.r,txtcolor.g,txtcolor.b)
             << move_to(w/2-(gout.twidth(szoveg)/2),koord.y+(mag/2)-5-(gout.cascent()+gout.cdescent())/2)
             << text(szoveg);                                                                                       //sz�veg

        int txtmag=gout.cascent()+gout.cdescent();
        gout.load_font("LiberationMono-Bold.ttf",w/50);
        gout << move_to(w/2-(gout.twidth(alcim)/2),koord.y+(mag/2)+(txtmag/2)-(gout.cascent()+gout.cdescent())/2)
             << text(alcim);                                                                                        //alc�m
    }
};

struct Gomb:public Textpanel        //kattinthat� gomb, neh�zs�gv�laszt�sra
{
    protected:
        bool jelolt=false;

    public:
    Gomb(int x_, int y_, string s, int r_, int g_, int b_): Textpanel(s)
    {
        szel=w/5;
        mag=h/10;
        koord.x=x_-szel/2;
        koord.y=y_-mag/2;
        txtcolor.r=r_;
        txtcolor.g=g_;
        txtcolor.b=b_;
    }

    void rajzol() override
    {
        gout.load_font("LiberationMono-Bold.ttf",w/28);
        if(jelolt)                                                                                                          //keret ha v�lasztott
            gout << color(txtcolor.r,txtcolor.g,txtcolor.b);
        else                                                                                                                //keret ha nem v�lasztott
            gout << color(67,67,67);
        gout << move_to(koord.x,koord.y) << box(szel,mag)
             << color(100,100,100) << move_to(koord.x+5,koord.y+5) << box(szel-10,mag-10)                                   //keret

             << color(txtcolor.r,txtcolor.g,txtcolor.b)
             << move_to(koord.x+(szel/2)-(gout.twidth(szoveg)/2),koord.y+(mag/2)-((gout.cascent()+gout.cdescent())/2))
             << text(szoveg);                                                                                               //sz�veg
    }

    void jelolvalt(bool b)              //jel�l�s/nemjel�l�s
    {
        jelolt=b;
    }

    int difficulty()                    //timer argumentum neh�zs�genk�nt
    {
        if(szoveg=="Hard")
            return 200;
        else if(szoveg=="Easy")
            return 400;
        else
            return 300;
    }

    szin colorker()
    {
        return txtcolor;
    }
};

struct Racs                             //r�cs az �tk�z�s vizsg�l�s�ra
{
    private:
        vector<vector<block>> grid;

    public:
    Racs()
    {
        grid=vector<vector<block>>(h/blocksize,vector<block>(w/blocksize));
        for(size_t i=0;i<grid.size();i++)
            for(size_t j=0;j<grid[i].size();j++)                                //koordin�t�k be�ll�t�sa
            {
                if(j==0)
                    grid[i][j].koord.x=0;
                else
                    grid[i][j].koord.x=grid[i][j-1].koord.x+blocksize;
                if(i==0)
                    grid[i][j].koord.y=0;
                else
                    grid[i][j].koord.y=grid[i-1][j].koord.y+blocksize;

                if(i<falvastag || j<falvastag || i>grid.size()-falvastag-1 || j>grid[i].size()-falvastag-1 ||  //oldalfalak
                   (i==falvastag+beltav && j>=falvastag+beltav && j<=grid[i].size()-(falvastag+beltav)-1) ||
                   (j==grid[i].size()/2 && i>=falvastag+beltav && i<=grid.size()-(falvastag+beltav)-1))        //bels� fal
                {
                    grid[i][j].status='f';
                }
            }
    }

    void hatter(canvas &hatter, canvas fu, canvas fal)                              //h�tt�r kirajzol�sa canvas-re a r�cs alapj�n
    {
        for(size_t i=0;i<grid.size();i++)
            for(size_t j=0;j<grid[i].size();j++)
            {
                if(grid[i][j].status=='f')
                    hatter << stamp(fal,grid[i][j].koord.x,grid[i][j].koord.y);
                else
                    hatter << stamp(fu,grid[i][j].koord.x,grid[i][j].koord.y);
            }
    }

    int xblockszam()
    {
        return grid[0].size();
    }

    int yblockszam()
    {
        return grid.size();
    }

    void testclear()                                                                //test st�tusz past st�tuszra, past st�tusz szabad st�tuszra �ll�t�sa
    {
        for(size_t i=0;i<grid.size();i++)
            for(size_t j=0;j<grid[i].size();j++)
            {
                if(grid[i][j].status=='t')
                    grid[i][j].status='p';
                else if(grid[i][j].status=='p')
                    grid[i][j].status='s';
            }
    }

    void testadd(pont testpoz)                                                      //testblockok r�gz�t�se a r�cson
    {
        for(size_t i=0;i<grid.size();i++)
            for(size_t j=0;j<grid[i].size();j++)
                if(grid[i][j].koord==testpoz)
                    grid[i][j].status='t';
    }

    char detect(pont fejpoz, char irany)                                            //k�vetkez� block st�tusz�nak visszak�ld�se
    {
        block* nextblock;
        bool volt=false;
        for(size_t i=0;i<grid.size();i++)
            for(size_t j=0;j<grid[i].size();j++)
            {
                if(grid[i][j].koord==fejpoz && !volt)
                {
                    if(irany=='j')
                        nextblock=&grid[i][j+1];
                    else if(irany=='f')
                        nextblock=&grid[i-1][j];
                    else if(irany=='l')
                        nextblock=&grid[i+1][j];
                    else
                        nextblock=&grid[i][j-1];
                    volt=true;
                }
            }
        if(nextblock->status=='a')                  //ha a k�vetkez� st�tusz alma, a st�tusz szabadra �ll�t�sa
        {
            nextblock->status='s';
            return 'a';
        }
        return nextblock->status;
    }

    void ghostmode(canvas c)                        //game over eset�n az utols� testblock visszarajzol�sa (hogy �gy t�nj�n, mintha nem mozogna a test)
    {
        for(size_t i=0;i<grid.size();i++)
            for(size_t j=0;j<grid[i].size();j++)
                if(grid[i][j].status=='p')
                    gout << stamp(c,grid[i][j].koord.x,grid[i][j].koord.y);
    }

    pont almakoord(pont fejpoz)                     //alma koordin�t�j�nak sorsol�sa, am�g szabad helyre nem ker�l
    {
        int i=rand()%grid.size(),j=rand()%grid[0].size();
        while(grid[i][j].status!='s' || grid[i][j].koord==fejpoz)
        {
            i=rand()%grid.size();
            j=rand()%grid[0].size();
        }
        grid[i][j].status='a';
        return grid[i][j].koord;
    }
};

struct Fej                                  //fej objektum
{
    private:
        pont koord;
        char irany='j';                     //j=jobb, b=bal, f=fel, l=le
        canvas textura;

    public:
    Fej(int xblock, int yblock, canvas c)
    {
        koord.x=(xblock/2+1)*20;
        koord.y=(yblock*20)-(falvastag*20+(beltav/2)*20);
        textura=c;
    }

    void rajzol()
    {
        gout << stamp(textura,koord.x,koord.y);
    }

    pont pozicio()
    {
        return koord;
    }

    char iranyker()
    {
        return irany;
    }

    void mozog(char c)
    {
        irany=c;
        if(irany=='j')
            koord.x+=blocksize;
        else if(irany=='f')
            koord.y-=blocksize;
        else if(irany=='l')
            koord.y+=blocksize;
        else
            koord.x-=blocksize;
    }

    void texreplace(canvas c)
    {
        textura=c;
    }
};

struct Test                 //test objektum
{
    private:
        pont koord;
        Test* follow;       //a k�vetetett testre mutat� pointer (minden block k�veti az el�tte l�v�t, kiv�ve az els�)
        canvas textura;

    public:
    Test(int x_, int y_, Test* p, canvas c)
    {
        koord.x=x_;
        koord.y=y_;
        follow=p;
        textura=c;
    }

    void rajzol()
    {
        gout << stamp(textura,koord.x,koord.y);
    }

    pont pozicio()
    {
        return koord;
    }

    void mozog(pont fejpoz)
    {
        if(follow==0)                   //els� block eset�n a fejet k�veti
        {
            koord.x=fejpoz.x;
            koord.y=fejpoz.y;
        }
        else                            //nem els� block eset�n az el�tte l�v� testet
        {
            koord.x=follow->koord.x;
            koord.y=follow->koord.y;
        }
    }
};

struct Alma                     //alma objektum
{
    private:
        pont koord;
        canvas textura;
        bool palyan=false;      //p�ly�n van-e �ppen az alma

    public:
    Alma(canvas c)
    {
        textura=c;
    }

    void rajzol()
    {
        if(palyan)
            gout << stamp(textura,koord.x,koord.y);
    }

    void makealma(pont bekoord)                         //alma p�ly�ra helyez�se
    {
        koord=bekoord;
        palyan=true;
    }

    void megesz()                                       //alma lev�tele a p�ly�r�l
    {
        palyan=false;
    }
};

void import(string fajl, canvas &c)                     //text�r�k beolvas�sa .kep f�jlokb�l, �s ment�s�k canvas-re
{
    ifstream be(fajl);
    vector<vector<szin>> kep;
    int szel, mag;
    be >> szel >> mag;                                  //sz�less�g, magass�g beolvas�sa az els� k�t sorb�l
    c.open(szel,mag);
    kep=vector<vector<szin>>(mag,vector<szin>(szel));
    for(size_t i=0;i<kep.size();i++)
        for(size_t j=0;j<kep[i].size();j++)
        {
            be >> kep[i][j].r >> kep[i][j].g >> kep[i][j].b;                            //rgb sz�nk�d beolvas�sa k�ppontonk�nt
            c << move_to(j,i) << color(kep[i][j].r,kep[i][j].g,kep[i][j].b) << dot;     //pont rajzol�sa canvas-re
        }
    be.close();
}

void gombrajzol(Title cim, vector<Gomb*> gombok, Textpanel start)   //f�men� kirajzol�sa (refresh-el egy�tt)
{
    cim.rajzol();   //c�m
    for(size_t i=0;i<gombok.size();i++)                             //neh�zs�g gombok
        gombok[i]->rajzol();
    start.rajzol();     //start
    gout << refresh;
}

void rajzol(canvas hatter, Fej fej, vector<Test*> testek, Alma alma, int pontszam)  //j�t�kt�r (minden ami j�t�kf�zisban szerepel) kirajzol�sa (refresh-el egy�tt)
{
    gout.load_font("LiberationMono-Bold.ttf",20);
    stringstream ss;
    gout << stamp(hatter,0,0);                          //h�tt�r
    for(size_t i=0;i<testek.size();i++)
        testek[i]->rajzol();                            //testblockok
    alma.rajzol();                                      //alma
    fej.rajzol();                                       //fej
    ss << "Points: " << pontszam;
    gout << color(0,255,0) << move_to((w/2)-gout.twidth(ss.str())/2,blocksize/2) << text(ss.str())  //pontsz�m
         << refresh;
}

int main()
{
    bool gamestart=true, gameover=false;    //j�t�kst�tuszt jelz� v�ltoz�k
    char inputirany='j',                    //beviteli ir�ny
         fejstatus;                         //k�vetkez� block st�tusz�nak t�rol�sa, ahova a fej l�pni fog
    int almakor=0,                          //k�r�k sz�ml�l�sa, ha el�ri az almatime-ot, alma elhelyez�se a p�ly�n
        pontszam=0;                         //pontsz�m
    const int almatime=8;                   //alma l�trehoz�s�nak gyakoris�ga (l�p�sben) (pl.: megev�st�l sz�m�tott 8. l�p�sben), v�ltoztathat�
    gout.open(w,h);

    canvas hatter,fejle,fejfel,fejjobb,fejbal,fejdead,test,fal,fu,almatex;      //canvas-ek text�r�k t�rol�s�ra
    import("fej2le.kep",fejle);                                                 //
    import("fej2fel.kep",fejfel);                                               //
    import("fej2jobb.kep",fejjobb);                                             //
    import("fej2bal.kep",fejbal);                                               //   Text�r�k
    import("fej2dead.kep",fejdead);                                             //  import�l�sa
    import("test.kep",test);                                                    //
    import("fal.kep",fal);                                                      //
    import("fu.kep",fu);                                                        //
    import("alma.kep",almatex);                                                 //

    Alma alma(almatex);                     //alma l�trehoz�sa
    Racs racs;                              //r�cs deklar�l�sa
    hatter.open(w,h);                       //   H�tt�r
    racs.hatter(hatter,fu,fal);             // l�trehoz�sa
    vector<Test*> testek;                                                                                  //  Testpointer-vektor �s fej
    Fej fej(racs.xblockszam(),racs.yblockszam(),fejjobb);                                                  //         deklar�l�sa
    testek.push_back(new Test(fej.pozicio().x-blocksize,fej.pozicio().y,0,test));                          //  Kezd� testblockok
    testek.push_back(new Test(testek[0]->pozicio().x-blocksize,testek[0]->pozicio().y,testek[0],test));    //   vektorhoz ad�sa
    rajzol(hatter,fej,testek,alma,pontszam);                                                                //j�t�kt�r kirajzol�sa (refresh-el egy�tt)

    vector<Gomb*> gombok;                                       //neh�zs�g gombpointer-vektor deklar�l�sa
    gombok.push_back(new Gomb(w/4,h/2,"Easy",0,255,0));         //
    gombok.push_back(new Gomb(w/2,h/2,"Normal",255,120,0));     //  3 neh�zs�gi gomb vektorhoz ad�sa
    gombok.push_back(new Gomb(w/4*3,h/2,"Hard",255,0,0));       //
    Gomb* jelolt=gombok[1];                                     //a kijel�lt gombra mutat� pointer
    jelolt->jelolvalt(true);                                    //normal be�ll�t�sa alapb�l
    Title cim("Epic Snake Game","or something");                //c�m deklar�l�sa
    Textpanel start("Start");                                   //startgomb deklar�l�sa
    gombrajzol(cim,gombok,start);                               //f�men� kirajzol�sa (refresh-el egy�tt)

    event ev;
    while(gin >> ev && ev.keycode!=key_escape) {                //escape eset�n kil�p�s
        if(gamestart)                                           //kiindul� f�zis (f�men�)
        {
            if(ev.type==ev_mouse && ev.button==btn_left)        //bal kattint�s eset�n
            {
                for(size_t i=0;i<gombok.size();i++)             //megnyomott neh�zs�g gomb kijel�l�se
                {
                    gombok[i]->jelolvalt(false);
                    if(gombok[i]->rajta(ev.pos_x,ev.pos_y))
                        jelolt=gombok[i];
                }
                jelolt->jelolvalt(true);
                start.colorchange(jelolt->colorker());          //keret sz�n�nek v�ltoztat�sa
                gombrajzol(cim,gombok,start);

                if(start.rajta(ev.pos_x,ev.pos_y))              //start eset�n j�t�kf�zis ind�t�sa
                {
                    gout.showmouse(false);                      //kurzor elt�ntet�se
                    gamestart=false;
                }
            }
            gin.timer(jelolt->difficulty());                    //timer be�ll�t�sa neh�zs�g alapj�n
        }

        else if(!gamestart && !gameover)                        //j�t�kf�zis
        {
            if(ev.type==ev_timer)                               //timer esem�ny eset�n
            {
                racs.testclear();                               //r�csr�l volt testek t�rl�se
                for(int i=testek.size()-1;i>=0;i--)             //testek mozgat�sa �s �j poz�ci� r�cson r�gz�t�se
                {
                    testek[i]->mozog(fej.pozicio());
                    racs.testadd(testek[i]->pozicio());
                }

                fejstatus=racs.detect(fej.pozicio(),inputirany);    //k�vetkez� block st�tusz�nak lek�r�se
                if(fejstatus=='f' || fejstatus=='t')                //�tk�z�s eset�n game over, elhunyt fej text�ra aktiv�l�sa
                {
                    fej.texreplace(fejdead);
                    gameover=true;
                }
                else                                            //k�l�nben fej mozgat�sa inputir�nynak megfelel�en
                {
                    fej.mozog(inputirany);
                    if(fejstatus=='a')                          //alm�val val� �tk�z�s eset�n eset�n
                    {
                        alma.megesz();                          //alma megev�se
                        testek.push_back(new Test(testek[testek.size()-1]->pozicio().x,testek[testek.size()-1]->pozicio().y,testek[testek.size()-1],test));     //testblock vektorhoz ad�sa
                        almakor=0;                              //k�rsz�ml�l� null�z�sa
                        pontszam+=100;                          //pontsz�m n�vel�se
                    }
                }

                almakor++;                                          //k�rsz�ml�l� l�ptet�se
                if(almakor==almatime)                               //almatime el�r�se eset�n alma l�trehoz�sa
                    alma.makealma(racs.almakoord(fej.pozicio()));

                rajzol(hatter,fej,testek,alma,pontszam);        //j�t�kt�r (h�tt�r, k�gy�, alma, pontsz�m) kirajzol�sa (refresh-el egy�tt)
                if(gameover)                                    //game over eset�n
                {
                    racs.ghostmode(test);                       //utols� testblock visszarajzol�sa (hogy a test l�tsz�lag ne mozogjon)
                    gout.showmouse(true);                       //kurzor megjelen�t�se
                    gout << refresh;
                }
            }

            else if(ev.type==ev_key)                                    //beviteli ir�ny olvas�sa, fejtext�ra ir�nynak megfelel� v�ltoztat�sa
            {
                if(ev.keycode==key_right && fej.iranyker()!='b')
                {
                    inputirany='j';
                    fej.texreplace(fejjobb);
                }
                else if(ev.keycode==key_up && fej.iranyker()!='l')
                {
                    inputirany='f';
                    fej.texreplace(fejfel);
                }
                else if(ev.keycode==key_down && fej.iranyker()!='f')
                {
                    inputirany='l';
                    fej.texreplace(fejle);
                }
                else if(ev.keycode==key_left && fej.iranyker()!='j')
                {
                    inputirany='b';
                    fej.texreplace(fejbal);
                }
            }
        }

        else                                                    //game over f�zis
        {
            Title ending("Game over!","rest in peace");         //game over �zenet
            Textpanel exit("Exit");                             //exit gomb
            exit.rajzol();
            ending.rajzol();
            gout << refresh;

            if(ev.type==ev_mouse && ev.button==btn_left && exit.rajta(ev.pos_x,ev.pos_y))       //exit gombra kattint�s eset�n kil�p�s
            {
                return 0;
            }
        }
    }
    return 0;
}
