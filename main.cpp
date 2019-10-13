#include<opencv2/opencv.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/aruco.hpp>

using namespace std;
using namespace cv;


Point2f corriger(Point2f place)
{
    int Dx, Dy;
    if (abs(place.x) <= 200)
        Dx = 0;
    else if(abs(place.x) <= 350 && abs(place.x) >= 200)
        Dx = 1;
    else
        Dx = 2;

    if (abs(place.y) <= 200)
        Dy = 0;
    else if(abs(place.y) <= 300 && abs(place.y)>= 200)
        Dy = 1;
    else
        Dy = 2;

    switch(Dx){
        case 0: place.x = place.x * 0.058;
            break;
        case 1: place.x = 200 * 0.058 + (place.x - 200) * 0.057;
            break;
        case 2: place.x = 200 * 0.058 + 150 * 0.057 + (place.x - 350) *0.056;
            break;
        default: cout<<"c'est faux";
    }
    switch(Dy){
        case 0: place.y = place.y * 0.058;
            break;
        case 1: place.y = 200 * 0.058 + (place.y - 200) * 0.057;
            break;
        case 2: place.y = 200 * 0.058 + 100 * 0.057 + (place.y - 300) *0.0556;
            break;
        default: cout<<"c'est faux";
    }
    return place;
}

vector<int> Bubble(vector<int> array, int n)
{
    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            if (array[i]>array[j]) {
                int temp = array[i];
                array[i] = array[j];
                array[j] = temp;
            }//每次i后面的元素比array[i]小就交换。
        }
    }
    return array;
}

vector<vector<Point2f> > mise_en_ordre(vector<vector<Point2f> > corners_rudimentaire, vector<int> id_rudimentaire)
{
    vector<vector<Point2f> > corners_new;
    vector<int> id_new;

    id_new = Bubble(id_rudimentaire, (int)id_rudimentaire.size());//id从小到大排序
    for (int i_new=0; i_new<id_new.size(); i_new++)
        for(int i_old=0; i_old<id_rudimentaire.size(); i_old++)
            if (id_new[i_new] == id_rudimentaire[i_old])
                corners_new.push_back(corners_rudimentaire[i_old]);//id从小到大，从原corners中依次找到对应的corner值并放入新的容器
    return corners_new;
}

vector<Point2f> getCoordonnees(Point2f ori, vector<vector<Point2f> > corners, vector<int> id)
{
    vector<Point2f> inrevised, revised;
    Point2f position;

    vector<vector<Point2f> > corners_mis = mise_en_ordre(corners, id);//将id按照从小到大顺序排列，
                                                                      // corners相应排序放进新的容器并返回新的corners容器
    for (int i = 0; i < id.size(); i++) {
        float points_x = 0.5 * ((corners_mis[i][0].x + 0.5 * (corners_mis[i][1].x - corners_mis[i][0].x)) +
                                (corners_mis[i][3].x + 0.5 * (corners_mis[i][2].x - corners_mis[i][3].x)));
        float points_y = 0.5 * ((corners_mis[i][0].y + 0.5 * (corners_mis[i][3].y - corners_mis[i][0].y)) +
                                (corners_mis[i][1].y + 0.5 * (corners_mis[i][2].y - corners_mis[i][1].y)));
                               //aruco相对于图片原点的横纵坐标
        position.x = points_x - ori.x;
        position.y = points_y - ori.y;//aruco相对于定义原点的横纵坐标
        inrevised.push_back(position);
        revised.push_back(corriger(inrevised[i]));//转化为现实的二维坐标系，原点为每一帧图片原点
    }
    return revised;
}//传回多个aruco码的坐标，以从小到大的id为排序

int main()
{
    VideoCapture inputVideo("/Users/mac/Desktop/ClionProjects/aruco_quatre/9.mov");
    //inputVideo.open(0);
    if(!inputVideo.isOpened())
        cout<<"fail to open!"<<endl;

    Ptr<aruco::Dictionary> dictionary = aruco::getPredefinedDictionary(aruco::DICT_6X6_250);
    Mat img, gray_img, edge_img, thres_img;
    vector<int> ids;
    vector<vector<int> > ids_mis;//mis用于存放排序后的ids
    vector<vector<Point2f> > coins;
    ofstream fileout;
    fileout.open("/Users/mac/Desktop/ClionProjects/aruco_quatre/output.txt");
    Point2f origin;
    vector<vector<Point2f> > finalOrbit;
    int totaltime;

    while (inputVideo.grab() && totaltime <= 500)
    {
        Mat image;
        inputVideo.retrieve(image);

        image.copyTo(img);
        cvtColor( img, gray_img, CV_RGB2GRAY );
        blur( gray_img, edge_img, Size(2,2) );
        //threshold(edge_img, thres_img, 100, 255, THRESH_BINARY);
        adaptiveThreshold(edge_img, thres_img, 255, ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 11, 2);

        origin.x = 0.5 * image.rows;
        origin.y = 0.5 * image.cols;
        aruco::detectMarkers(thres_img, dictionary, coins, ids);

        ids_mis.push_back(Bubble(ids, (int)ids.size()));
        finalOrbit.push_back(getCoordonnees(origin, coins, ids));//得到第n帧一组aruco的坐标并存储

        aruco::drawDetectedMarkers(thres_img, coins, ids);
        imshow("out", thres_img);
        waitKey(50);

        if (waitKey(33) == 27)
                break;
        totaltime++;
    }
    for (int frame = 0; frame < finalOrbit.size(); frame++)
    {   cout<<endl;
        for (int toutes_les_ids = 0; toutes_les_ids < finalOrbit.at(frame).size(); toutes_les_ids++)
        {
            fileout << "time * 100ms" << ' ' << ' ' << frame << ' ' << ' ';
            fileout << "l'identite est" << ' ' << ' ' << ids_mis[frame][toutes_les_ids] << ' ' << ' ';
            fileout << "les coordonnees correspondants sont" << ' ' << ' '
                    << finalOrbit.at(frame).at(toutes_les_ids).x << ' ' << finalOrbit.at(frame).at(toutes_les_ids).y << endl;
        }
    }
    fileout << flush;
    fileout.close();
}