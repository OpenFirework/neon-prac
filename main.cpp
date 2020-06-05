#define __ARM_NEON 1
#include <iostream>
#include <fstream>
#include <string.h>
#include <arm_neon.h>
#include <math.h>

int32_t NowMicros() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return static_cast<int32_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

using namespace std;

double ncnn_normal(const float* feature1, const float* feature2){
     float sum = 0.f;
     int channels = 1; 
     int size = 512;
     const float* w = feature1;
     
     float32x4_t _sum = vdupq_n_f32(0.f);
     float32x4_t _sum2 = vdupq_n_f32(0.f);

		const float* m = feature2;
    int nn = size >> 3;
    asm volatile( 
								 "0: \n" 
								 "pld [%1, #256] \n" 
								 "vld1.f32 {d0-d3}, [%1 :128]! \n" 
								 "pld [%2, #256] \n" 
								 "vld1.f32 {d4-d7}, [%2]! \n" 
								 "vmla.f32 %q3, q0, q2 \n" 
								 "subs %0, #1 \n" 
								 "vmla.f32 %q4, q1, q3 \n" 
								 "bne 0b \n" 
								 : "=r"(nn), // %0 
								 "=r"(m), // %1 
								 "=r"(w), // %2 
								 "=w"(_sum), // %3 
								 "=w"(_sum2) // %4 
								 : "0"(nn), 
								 "1"(m), 
								 "2"(w), 
								 "3"(_sum), 
								 "4"(_sum2) 
								 : "cc", "memory", "q0", "q1", "q2", "q3" 
		);
    	
	 _sum = vaddq_f32(_sum, _sum2); 

	 float32x2_t _sumss = vadd_f32(vget_low_f32(_sum), vget_high_f32(_sum)); 
	 _sumss = vpadd_f32(_sumss, _sumss); 
	 sum += vget_lane_f32(_sumss, 0); 

 return sum * 0.5f + 0.5f;
}



float FaceVerify(const float* feature1, const float* feature2) {
  float simi = 0;
 
  for (int i = 0; i <512; ++i) {
    simi += feature1[i] * feature2[i];
  }
   return (simi + 1.0) / 2.0 ;
}
void normalize(const float *in_feature, float *out_feature) {
		float ssum = 0.0;
    for (int j = 0; j < 512; j++) {       
      ssum += in_feature[j] * in_feature[j];
    }
    
    ssum = 1.f / sqrt(ssum);
    for (int j = 0; j < 512; j++) {       
      out_feature[j] = in_feature[j] * ssum;
    }
}

void neon_normalize(float *in_feature, float *out_feature) {
	 float ssum = 1e-6;
	 int size = 512;
   int nn = size >> 3;
   float* m = in_feature;
   float* w = out_feature;
   float* z = in_feature;
    printf("%0x\n",m);
   asm (
   	 "vmov.f32 q4, #0.0 \n"
     "vmov.f32 q5, #0.0 \n"
     "0: \n"
     "pld [%1, #256] \n" 
     "vld1.f32 {d0-d3}, [%1] \n"
     "pld [%1, #256] \n" 
 	   "vld1.f32 {d4-d7}, [%1]! \n"
 	   "vmla.f32 q4, q0, q2\n"
 	   "subs %0, #1 \n"
 	   "vmla.f32 q5, q1, q3\n"  
     "bne 0b \n"
     "vadd.f32 q4, q4, q5\n"
     "vpadd.f32 d0, d8, d9 \n"
     "vpadd.f32 d9, d0, d0 \n"
     "vrsqrte.f32 d10, d9 \n"
     "vmov.f32 %3, d10[0] \n"
     "adds %0, #64 \n"
     "1: \n"
     "pld [%4, #256] \n"     
     "vld1.f32 {d0-d3}, [%4]! \n"
     "pld [%4, #256] \n" 
 	   "vld1.f32 {d4-d7}, [%4]! \n"
 	   "vmul.f32 q0, q0, d10[0] \n"
 	   "vst1.f32 {d0-d1}, [%2]! \n"
 	   "vmul.f32 q1, q1, d10[0] \n"
	   "vst1.f32 {d2-d3}, [%2]! \n"
 	   "vmul.f32 q2, q2, d10[0] \n"
	   "vst1.f32 {d4-d5}, [%2]! \n"
 	   "vmul.f32 q3, q3, d10[0] \n"
 	   "vst1.f32 {d6-d7}, [%2]! \n"
 	   "subs %0, #1 \n"
     "bne 1b \n"
    :"=r"(nn), // %0
     "=r"(m),
     "=r"(w),
     "=r"(ssum),
     "=r"(z)
    :"0"(nn),
     "1"(m),  
     "2"(w),
     "3"(ssum),
     "4"(z)  
    : "cc", "memory", "q0", "q1", "q2", "q3", "q4","q5","q6","q7","q8","q9"
   );

}

float neon_FaceVerify(const float* feature1, const float* feature2) {
  float simi = 0;
  
  float *sum = new float[4];
  int size = 512;
  const float* w = feature1;
  const float* m = feature2;
  
  int nn = size >> 3;
   asm (
     "vmov.f32 q4, #0.0 \n"
     "vmov.f32 q5, #0.0 \n"
     "0: \n"
     "pld [%2, #256] \n" 
     "vld1.f32 {d0-d3}, [%2]! \n"
     "pld [%3, #256] \n" 
 	   "vld1.f32 {d4-d7}, [%3]! \n"
 	   "vmla.f32 q4, q0, q2\n"
 	   "subs %0, #1 \n"
 	   "vmla.f32 q5, q1, q3\n"  
     "bne 0b \n"
     "vadd.f32 q4, q4, q5\n"
     "vst1.f32 q4, [%1] \n"
    :"=r"(nn), // %0
     "=r"(sum), // %1
     "=r"(m),
     "=r"(w)
    :"0"(nn),
     "1"(sum),
     "2"(m),  
     "3"(w)   
    : "cc", "memory", "q0", "q1", "q2", "q3", "q4","q5"
  );
  
  for(int i=0;i<4;i++) {
    simi += sum[i];
  }
  return (simi + 1.0) / 2.0 ;
}

int main() {

// test load

	//int * gray = (int*)memalign(4,8*sizeof(int));
	//int * dst = (int*)memalign(4,8*sizeof(int));
	int * gray = new int[8];
	int * dst = new int[8];
	for(int i=0;i<8;i++) {
	  gray[i] = i;
	}
	
	memset(dst, 0, 8*sizeof(int));

	asm volatile(
		"vld1.f32 {d0-d3}, [%0] \n"
		"vst1.f32 {d0-d3}, [%1] \n"
		:"=r"(gray),
     "=r"(dst)
		:"0"(gray),
		 "1"(dst)
	);

  
	
	ifstream infile("/data/liujiangkuan/neon_test/feature_1.bin",std::ios::binary|std::ios::in);
  float in_1[512*100];
  while(infile.read((char*)in_1,512*100*sizeof(float))) {

  }
	infile.close();
	
	infile.open("/data/liujiangkuan/neon_test/feature_2.bin",std::ios::binary|std::ios::in);
  float in_2[512*100];
  while(infile.read((char*)in_2,512*100*sizeof(float))) {

  }
	infile.close();
	
	float *value = (float*)memalign(16,32*sizeof(float));
	float *temp = (float*)memalign(16,32*sizeof(float));
	float *result = (float*)memalign(16,32*sizeof(float));
	memset(result,0,sizeof(float)*32);
	for(int i=0;i<32;i++) {
	  value[i] = i;
	  temp[i] = i;
	}
	//32个64-bit寄存器(D0-D31)
	//16个128-bit寄存器(Q0-Q15)
	asm(
	   "vld1.f32 {d0-d3}, [%1]! \n"
	   "vld1.f32 {d4-d7}, [%2]! \n"
	   "vmul.f32 q4, q0, q2 \n"
	   "vmla.f32 q4, q4, q0 \n"
	   "vst1.f32 {d8-d11}, [%0] \n"
	   :"=r"(result)
	   :"r"(value),
	    "r"(temp)
	);
	
	
	float *out_feature = new float[512];
	float *out_feature_1 = new float[512];
	
	int start = 0;
	int end = 0;

	//printf("%0x\n",out_feature);
	
	neon_normalize(in_2,out_feature);

	normalize(in_1+128,out_feature_1);

	normalize(in_2,out_feature);
	
	
	float score; 
	start = NowMicros();
	for(int i=0;i<100;i++)
	score = FaceVerify(out_feature,out_feature_1);
	end = NowMicros();
	printf("%dus\n",(end-start));
	cout<<score<<endl;
	
	start = NowMicros();
	for(int i=0;i<100;i++)
	score = neon_FaceVerify(out_feature,out_feature_1);
	end = NowMicros();
	
	printf("%dus\n",(end-start));
	
	start = NowMicros();
	for(int i=0;i<100;i++)
	score = ncnn_normal(out_feature,out_feature_1);
	end = NowMicros();
	printf("%dus\n",(end-start));
	
	
  //float *sum =  (float*)memalign(16,4*sizeof(float));
  /*
  float *sum = new float[4];
	
	int start = 0;
	int end = 0;
	int all = 0;
	float score;
	for(int j=0;j<1;j++) {
	  float *temp = new float[512*100];
	  float *temp_1 = new float[512*100];
	  memcpy(temp, in_1, sizeof(float)*512*100);
	  memcpy(temp_1, in_2, sizeof(float)*512*100);
		start = NowMicros();
		for(int z=0;z<1000;z++)
		for(int k=0;k<100;k++)
		score = FaceVerify(temp+512*k, temp_1+512*k);
		
	 end = NowMicros();
	 delete[] temp;
	 delete[] temp_1;
	 all = all+(end-start);
	}
	
	 		
	cout<<glob<<endl;
	
	printf("%dus\n",all);
	printf("%f\n",score);
	all = 0;
	//float *w = (float*)memalign(4,512*sizeof(float));
 	//float *m = (float*)memalign(4,512*sizeof(float));
 	float *w = new float[512];
 	float *m = new float[512];
     memcpy(w, in_1, sizeof(float)*512);
	  memcpy(m, in_2, sizeof(float)*512);
 
  	for(int j=0;j<1;j++) {
  		float *temp = new float[512*100];
			float *temp_1 = new float[512*100];
			memcpy(temp, in_1, sizeof(float)*512*100);
			memcpy(temp_1, in_2, sizeof(float)*512*100);
			start = NowMicros();
			for(int z=0;z<1000;z++)
		  for(int k=0;k<100;k++)
			score = neon_FaceVerify(temp+512*k, temp_1+512*k, sum);
			
			end = NowMicros();
			
			all = all+(end-start);
  	
  	}
	 
  
	
    
	
	printf("%dus\n",all);
	printf("%f\n",score);
	*/
	return 0;
}
