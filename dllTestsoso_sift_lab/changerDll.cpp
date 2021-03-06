#include "changerDll.hpp"

Mat getMyGabor(int width, int height, int U, int V, double Kmax, double f,  
    double sigma, int ktype, const string& kernel_name)  
{  
    //CV_ASSERT(width % 2 == 0 && height % 2 == 0);  
    //CV_ASSERT(ktype == CV_32F || ktype == CV_64F);  
  
    int half_width = width / 2;  
    int half_height = height / 2;  
    double Qu = PI*U/8;  
    double sqsigma = sigma*sigma;  
    double Kv = Kmax/pow(f,V);  
    double postmean = exp(-sqsigma/2);  
  
    Mat kernel_re(width, height, ktype);  
    Mat kernel_im(width, height, ktype);  
    Mat kernel_mag(width, height, ktype);  
  
    double tmp1, tmp2, tmp3;  
    for(int j = -half_height; j <= half_height; j++){  
        for(int i = -half_width; i <= half_width; i++){  
            tmp1 = exp(-(Kv*Kv*(j*j+i*i))/(2*sqsigma));  
            tmp2 = cos(Kv*cos(Qu)*i + Kv*sin(Qu)*j) - postmean;  
            tmp3 = sin(Kv*cos(Qu)*i + Kv*sin(Qu)*j);  
  
            if(ktype == CV_32F)  
                kernel_re.at<float>(j+half_height, i+half_width) =   
                    (float)(Kv*Kv*tmp1*tmp2/sqsigma);  
            else  
                kernel_re.at<double>(j+half_height, i+half_width) =   
                    (double)(Kv*Kv*tmp1*tmp2/sqsigma);  
  
            if(ktype == CV_32F)  
                kernel_im.at<float>(j+half_height, i+half_width) =   
                    (float)(Kv*Kv*tmp1*tmp3/sqsigma);  
            else  
                kernel_im.at<double>(j+half_height, i+half_width) =   
                    (double)(Kv*Kv*tmp1*tmp3/sqsigma);  
        }  
    }   
    magnitude(kernel_re, kernel_im, kernel_mag);  
    if(kernel_name.compare("real") == 0)  
        return kernel_re;  
    else if(kernel_name.compare("imag") == 0)  
        return kernel_im;  
    else if(kernel_name.compare("mag") == 0)  
        return kernel_mag;  
    else  
        cout<<"Invalid kernel name!"<<std::endl;  
}  
Mat gabor_filter(Mat& img,int dir,int scale,int kernelSize,int type)  
{  
    // variables for gabor filter  
    double Kmax = PI/2;  
    double f = sqrt(2.0);  
    double sigma = 2*PI;  
    int U = dir;  
    int V = 4;  
    int GaborH = kernelSize;  
    int GaborW = kernelSize;  
  
    //   
    Mat kernel_re, kernel_im;  
    Mat dst_re, dst_im, dst_mag;  
  
    // variables for filter2D  
    Point archor(-1,-1);  
    int ddepth = -1;  
    double delta = 0;  
  
    // filter image with gabor bank  
            kernel_re = getMyGabor(GaborW, GaborH, U, V,  
                Kmax, f, sigma, CV_64F, "real");  
            kernel_im = getMyGabor(GaborW, GaborH, U, V,  
                Kmax, f, sigma, CV_64F, "imag");  
  
            filter2D(img, dst_re, ddepth, kernel_re);  
            filter2D(img, dst_im, ddepth, kernel_im);  
  
            dst_mag.create(img.rows, img.cols, CV_32FC1);
            magnitude(Mat_<float>(dst_re),Mat_<float>(dst_im),   
                dst_mag);  
  
            //show gabor kernel  
            normalize(dst_mag, dst_mag, 0, 1, CV_MINMAX);  
 
    return dst_mag;  
}  
Mat GetMyHist(Mat image,bool IsShow,int histSize)
{

    Mat dst;
	MatND hist;
    image.convertTo(dst, CV_8U,1, 0);    
    calcHist(&dst, 1, 0, Mat(), hist, 1, &histSize, 0);
	if(IsShow)
   {
	   imshow("image", dst); 
	   Mat histImage = Mat::ones(200, 320, CV_8U)*255;
		normalize(hist, hist, 0, histImage.rows, CV_MINMAX, CV_32F);
		histImage = Scalar::all(255);
		int binW = cvRound((double)histImage.cols/histSize);
	 
		for( int i = 0; i < histSize; i++ )
			rectangle( histImage, Point(i*binW, histImage.rows),
					   Point((i+1)*binW, histImage.rows - cvRound(hist.at<float>(i))),
					   Scalar::all(0), -1, 8, 0 );
		imshow("histogram", histImage);
		waitKey(0);
	}
	 return hist;

}
/*************************************************************************************** 
Function:  区域生长算法 
Input:     src 待处理原图像 pt 初始生长点 th 生长的阈值条件 
Output:    肺实质的所在的区域 实质区是白色，其他区域是黑色 
Description: 生长结果区域标记为白色(255),背景色为黑色(0) 
Return:    Mat 
Others:    NULL 
***************************************************************************************/  
RGN  RegionGrow(Mat src, Point2i pt, int th,int label,Mat& matDst)  
{  
    Point2i ptGrowing;                      //待生长点位置  
    int nGrowLable = 0;                             //标记是否生长过  
    int nSrcValue = 0;                              //生长起点灰度值  
    int nCurValue = 0;                              //当前生长点灰度值  
 //   Mat matDst = Mat::zeros(src.size(), CV_8UC1);   //创建一个空白区域，填充为黑色  
    //生长方向顺序数据  
    int DIR[8][2] = {{-1,-1}, {0,-1}, {1,-1}, {1,0}, {1,1}, {0,1}, {-1,1}, {-1,0}};    
    Vector<Point2i> vcGrowPt;                     //生长点栈  
    vcGrowPt.push_back(pt);                         //将生长点压入栈中  
    matDst.at<uchar>(pt.y, pt.x) = 255;               //标记生长点  
    nSrcValue = src.at<uchar>(pt.y, pt.x);            //记录生长点的灰度值  
	RGN rgnL = {0,0,0,src.cols,src.rows,0,0};
	int nGrows = 0;
    while (!vcGrowPt.empty()&&nGrows<MAXGROWS)                       //生长栈不为空则生长  
    {  
        pt = vcGrowPt.back();                       //取出一个生长点  
        vcGrowPt.pop_back();                            
        //分别对八个方向上的点进行生长  
        for (int i = 0; i<9; ++i)  
        {  
            ptGrowing.x = pt.x + DIR[i][0];       
            ptGrowing.y = pt.y + DIR[i][1];   
            //检查是否是边缘点  
            if (ptGrowing.x < 0 || ptGrowing.y < 0 || ptGrowing.x > (src.cols-1) || (ptGrowing.y > src.rows -1))  
                continue;  
  
            nGrowLable = matDst.at<uchar>(ptGrowing.y, ptGrowing.x);      //当前待生长点的灰度值  
  
            if (nGrowLable == 0)                    //如果标记点还没有被生长  
            {  
                nCurValue = src.at<uchar>(ptGrowing.y, ptGrowing.x);            
                if (abs(nSrcValue - nCurValue) <= th)                 //在阈值范围内则生长  
                {  
                    matDst.at<uchar>(ptGrowing.y, ptGrowing.x) = label;     //标记为标记值  
                    vcGrowPt.push_back(ptGrowing);                  //将下一个生长点压入栈中  
					rgnL.sz++;
					rgnL.left = rgnL.left>ptGrowing.x?ptGrowing.x:rgnL.left;
					rgnL.right = rgnL.right<ptGrowing.x?ptGrowing.x:rgnL.right;
					rgnL.top = rgnL.top>ptGrowing.y?ptGrowing.y:rgnL.top;
					rgnL.bottom = rgnL.bottom<ptGrowing.y?ptGrowing.y:rgnL.bottom;
					rgnL.cx = rgnL.cx + ptGrowing.x;
					rgnL.cy = rgnL.cy + ptGrowing.y;
					nGrows++;
                }  
            }  
        }  
    } 
	if(rgnL.sz>0)
	{
		rgnL.cx = rgnL.cx/rgnL.sz;
		rgnL.cy = rgnL.cy/rgnL.sz;
	} 
   return rgnL; 
}  

vector<Rect> ImageLabel(Mat srcImg,int szTh, Mat& dstImg)
{
	  Point2i pt;
	  int initLabel = 90;// 标记初始值
	  int label = initLabel;
	  int cnt  = 0;
	  int i,j;
	  RGN rgnCandt = {0,0,0,0,0,0,0};
	  vector<Rect> RectOutput;
	  for(i = 0;i< srcImg.rows;i++)
		 for(j = 0;j< srcImg.cols;j++)
	  {
		  if(srcImg.at<uchar>(i,j)==255)
		  {
			  pt.x = j;
			  pt.y = i;
			  rgnCandt = RegionGrow(srcImg,pt,0,label,dstImg);
			  if(rgnCandt.sz>szTh)
			  {
				  label++;
				  Rect brect;
				  brect.x = rgnCandt.left;
				  brect.y = rgnCandt.top;
				  brect.width = rgnCandt.right -  rgnCandt.left;
				  brect.height = rgnCandt.bottom - rgnCandt.top;
				  RectOutput.push_back(brect);
			  }
		  }
	  }
      return RectOutput;
}
//Mat srcImgA --源图像A
//Mat srcImgB --源图像B
//DETECT_PARAM params--检测参数
//vector<Rect> output --输出变化区域外接矩形框

void ChangeDetector(Mat srcImgA,Mat srcImgB,DETECT_PARAM params,vector<Rect>& output,vector<float>& outputCorrelationCf,string debugFileName)
{
	 //求sobel边缘图像	 /// Generate grad_x and grad_y
	 int imgWidth = srcImgA.cols;
	 int imgHeight = srcImgA.rows;
	 int imgSz = imgWidth*imgHeight;
	 int scaleSobel = 1;
	 int delta = 0;
	 int ddepth = CV_16S;
	 int i,j,k;
	 int scale = params.nScale;//3;
	 int kernelSize= params.nSzKernel;//7;
	 int szTh = params.nChangeRnSz;
	 int type = 1;
	 int nFilters = params.nFilters;
	 float sensCoff = params.fSensitivity;//0.01;
	 vector<float> edt_a;
	 vector<float> edt_b;
	 float a_c = 0,b_c = 0,cross_c = 0;
	 float mean_edt_a = 0,mean_edt_b = 0;
	 float temp = 0;
	 float tempA = 0;
	 float tempB = 0;
	 float delta_miu_min = 10000;
	 float delta_miu_max = -10000;
	 float delta_miu_range = 0;
	double maxValue = 255 ; 
	  int adaptiveMethod = ADAPTIVE_THRESH_GAUSSIAN_C;// CV_ADAPTIVE_THRESH_MEAN_C;
      int thresholdType = CV_THRESH_BINARY; 
	double C = 5;	
	 long  accum = 0;
	 float nm = 0;
	 uchar th_cf = 0;
	 Point archor(-1,-1); 

	 Mat grad_x, grad_y;
	 Mat grad_A, grad_B;
	 Mat abs_grad_x, abs_grad_y;
	 vector<Mat> vector_filterd_image_A;
	 vector<Mat> vector_filterd_image_B;
	 Mat filterd_image;
	 Mat cf_weight_8u(imgHeight,imgWidth,CV_8U);
	 Mat hist;
	 Mat cf(imgHeight,imgWidth,CV_32FC1);
	 Mat delta_miu(imgHeight,imgWidth,CV_32FC1);
	 Mat cf_weight(imgHeight,imgWidth,CV_32FC1);
	 Mat cf_weight_bw;
	 Mat dispImg;
	 Mat dstImg;
	 /// Gradient X
	 //Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );
	 Sobel( srcImgA, grad_x, ddepth, 1, 0, 3, scaleSobel, delta, BORDER_DEFAULT );
	 convertScaleAbs( grad_x, abs_grad_x );
	 /// Gradient Y
	 //Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
	 Sobel( srcImgA, grad_y, ddepth, 0, 1, 3, scaleSobel, delta, BORDER_DEFAULT );
	 convertScaleAbs( grad_y, abs_grad_y );
	 /// Total Gradient (approximate)
	 addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad_A );
	 //imshow( "边缘图像A", grad_A );
	 //waitKey(0);
	 Sobel( srcImgB, grad_x, ddepth, 1, 0, 3, scaleSobel, delta, BORDER_DEFAULT );
	 convertScaleAbs( grad_x, abs_grad_x );
	 /// Gradient Y
	 //Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
	 Sobel( srcImgB, grad_y, ddepth, 0, 1, 3, scaleSobel, delta, BORDER_DEFAULT );
	 convertScaleAbs( grad_y, abs_grad_y );
	 /// Total Gradient (approximate)
	 addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad_B );
	 ////求gabor滤波
     for( i = 0;i<nFilters;i++)
     {
  	   filterd_image = gabor_filter(grad_A,i , scale, kernelSize, type);
	   vector_filterd_image_A.push_back(filterd_image);
  	   filterd_image = gabor_filter(grad_B,i , scale, kernelSize, type);
	   vector_filterd_image_B.push_back(filterd_image);
     }
	 for(i = 0;i<imgHeight;i++)
      for(j = 0;j<imgWidth;j++)
	  {
		  mean_edt_a = 0;
		  mean_edt_b = 0;
		  a_c = 0;
		  b_c = 0;
		  cross_c = 0;
 		 for(k=0;k<nFilters;k++)
         {
			 filterd_image=vector_filterd_image_A.at(k);
			 edt_a.push_back(filterd_image.at<float>(i,j));
			 filterd_image=vector_filterd_image_B.at(k);
			 edt_b.push_back(filterd_image.at<float>(i,j));
			 mean_edt_a = mean_edt_a+edt_a.at(k);
			 mean_edt_b = mean_edt_b+edt_b.at(k);
		 }
		 mean_edt_a = mean_edt_a/nFilters;
		 mean_edt_b = mean_edt_b/nFilters;

		 for(k=0;k<nFilters;k++)
		 {
		   tempA = edt_a.at(k);
		   tempB = edt_b.at(k);
	
		   a_c = a_c +  (tempA - mean_edt_a)*(tempA - mean_edt_a);
		   b_c = b_c +  (tempB - mean_edt_b)*(tempB - mean_edt_b);
		   cross_c = cross_c + (tempA - mean_edt_a)*(tempB - mean_edt_b);
		 }
		 edt_a.clear();
  	     edt_b.clear();
		 cf.at<float>(i,j) = cross_c/(sqrt(a_c*b_c)+0.001);
		 temp = abs(mean_edt_a - mean_edt_b);
		 delta_miu.at<float>(i,j) = temp;
		 delta_miu_min = delta_miu_min>temp?temp:delta_miu_min;
		 delta_miu_max = delta_miu_max<temp?temp:delta_miu_max;
	  }

      delta_miu_range = 1/(delta_miu_max-delta_miu_min + 0.001);
  	  for(i = 0;i<imgHeight;i++)
      for(j = 0;j<imgWidth;j++)
	  {
		  cf_weight.at<float>(i,j) = delta_miu_range*delta_miu.at<float>(i,j)*(1-cf.at<float>(i,j));
	  }
	  normalize(cf_weight,cf_weight,0,255,CV_MINMAX);
	  cf_weight.convertTo(cf_weight_8u,CV_8U,1,0);
	  hist = GetMyHist(cf_weight_8u,false,256);
	  for(i = hist.rows-1;i > 0; i--)
      {
		  accum = accum + hist.at<float>(i);
		  if(accum>imgSz*sensCoff)
		  {
			  th_cf = i;
			  break;
		  }
	  }
     // Scalar mean_cf_weight_8u = mean(cf_weight_8u,noArray());
	  dilate(cf_weight_8u,cf_weight_8u,Mat(),archor,1, BORDER_CONSTANT, morphologyDefaultBorderValue());
	  erode(cf_weight_8u,cf_weight_8u, Mat(), archor,1, BORDER_CONSTANT, morphologyDefaultBorderValue());
	  threshold(cf_weight_8u, cf_weight_bw, th_cf, 255, THRESH_BINARY);
//test by zt on 0909
imwrite(debugFileName,cf_weight_bw);
//waitKey(0);
	  dstImg = Mat::zeros(cf_weight_bw.size(), CV_8UC1);
	  dispImg = srcImgA.clone();
      output = ImageLabel(cf_weight_bw,szTh,dstImg);
	  //获取output的相关系数
	  float sumLocal = 0;
	  
	  for(k = 0;k<output.size();k++)
	  {
		  Rect rc = output.at(k);
		  sumLocal = 0;
		  for(i = rc.y;i<(rc.y+rc.height);i++)
	       for(j = rc.x;j<(rc.x+rc.width);j++)
		   {
		       sumLocal = sumLocal+(cf_weight.at<float>(i,j))/255;
		   }
		   sumLocal = sumLocal/(rc.height*rc.width);
		   outputCorrelationCf.push_back(sumLocal);
	  }
}
void ChangeDetectorKH(Mat srcImgA,Mat srcImgB,DETECT_PARAM params,vector<Rect>& output,vector<float>& outputCorrelationCf)
{
         int imgWidth = srcImgA.cols;
	 int imgHeight = srcImgA.rows;
	 int i,j;
	 int m,n;
	 int th = int(params.fSensitivityKH);
	 int szTh = params.nChangeRnSzKH;

	 Mat gaussImgA(imgHeight,imgWidth,CV_32FC1);
	 Mat gaussImgB(imgHeight,imgWidth,CV_32FC1);
	 Mat diffGaussImg(imgHeight,imgWidth,CV_32FC1);
	 Mat dstImg = Mat::zeros(diffGaussImg.size(), CV_8UC1);
	 Mat diffGaussImgU(imgHeight,imgWidth,CV_8UC1);
	 Point archor(-1,-1); 
	 float c1 = 0.025174000;
	 float c2 = 0.001715000;
	 float c3 = -0.02396500;
	 for(i = 0;i<imgHeight;i++)
         for(j = 0;j<imgWidth;j++)
	 {
		 gaussImgA.at<float>(i,j) = c1*srcImgA.at<cv::Vec3b>(i,j)[0]+c2*srcImgA.at<cv::Vec3b>(i,j)[1]+c3*srcImgA.at<cv::Vec3b>(i,j)[2];
		 gaussImgB.at<float>(i,j) = c1*srcImgB.at<cv::Vec3b>(i,j)[0]+c2*srcImgB.at<cv::Vec3b>(i,j)[1]+c3*srcImgB.at<cv::Vec3b>(i,j)[2];
	 }
	// normalize(gaussImgA, gaussImgA, 0, 1, CV_MINMAX);
	// normalize(gaussImgB, gaussImgB, 0, 1, CV_MINMAX);
	 diffGaussImg = abs(gaussImgB-gaussImgA);
	// normalize(diffGaussImg, diffGaussImg, 0, 1, CV_MINMAX);
	// diffGaussImg = diffGaussImg*255;
	// imwrite("diffGaussImg.jpg",diffGaussImg);  
	 threshold(diffGaussImg, diffGaussImg, th, 255, THRESH_BINARY);	
	// imwrite("diffGaussImgBW.jpg",diffGaussImg);  
	 dilate(diffGaussImg,diffGaussImg,Mat(),archor,1, BORDER_CONSTANT, morphologyDefaultBorderValue());
	 erode(diffGaussImg,diffGaussImg, Mat(), archor,1, BORDER_CONSTANT, morphologyDefaultBorderValue());
         diffGaussImg.convertTo(diffGaussImgU,CV_8U,1,0);
	
	// imshow("diffGaussImgU",diffGaussImgU);
	// waitKey(0);
	// imwrite("diffGaussImgU.jpg",diffGaussImgU);
	 output = ImageLabel(diffGaussImgU,szTh,dstImg);

}
//color balance
void colorCorrect(Mat srcImgA,Mat srcImgB,Mat dstImgA,Mat dstImgB)
{
  // Mat integralImgA;
   vector<Mat> rImgA(srcImgA.channels());
   vector<Mat> rImgB(srcImgB.channels());
   Mat grayImgA,grayImgB,labImgA,labImgB;
//   cvtColor(srcImgA, grayImgA, CV_BGR2GRAY);
//   cvtColor(srcImgB, grayImgB, CV_BGR2GRAY);
 
   cvtColor(srcImgA, labImgA, CV_BGR2Lab);      
   cvtColor(srcImgB, labImgB, CV_BGR2Lab);

   split(labImgA,rImgA);
   split(labImgB,rImgB);
   //DETECT_PARAM params;
   DETECT_PARAM params = {3,7,8,400,0.01,200,1.6,5,0.2};
   vector<Rect> output;
   vector<float> outputCorrelationCf;
//  ChangeDetector(grayImgA,grayImgB,params,output,outputCorrelationCf,"/home/guangdian/zhangtao/ftp/DebugResult/cf_bw_gray.jpg");
//cout<<"save cf_bw_gray ok"<<endl;
   ChangeDetector(rImgA[0],rImgB[0],params,output,outputCorrelationCf,"/home/guangdian/zhangtao/ftp/DebugResult/cf_bw_L.jpg");
//cout<<"save cf_bw_L ok"<<endl;
   ChangeDetector(rImgA[1],rImgB[1],params,output,outputCorrelationCf,"/home/guangdian/zhangtao/ftp/DebugResult/cf_bw_a.jpg");
//cout<<"save cf_bw_a ok"<<endl;
   ChangeDetector(rImgA[2],rImgB[2],params,output,outputCorrelationCf,"/home/guangdian/zhangtao/ftp/DebugResult/cf_bw_b.jpg");
//cout<<"save cf_bw_b ok"<<endl;
 //  integral(grayImgA,integralImgA);
 //  integral(grayImgB,integralImgB);

 //  imshow("A l image",rImgB[0]);
 //  waitKey(0);
 //  imshow("B a image",rImgB[1]);
 //  waitKey(0);
 //  imshow("B b image",rImgB[2]);
 //  waitKey(0);
 //  imshow("A l image",rImgA[0]);
 //  waitKey(0);
 //  imshow("A a image",rImgA[1]);
 //  waitKey(0);
 //  imshow("A b image",rImgA[2]);
 //  waitKey(0);

}

