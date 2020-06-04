#define __ARM_NEON 1
#include <iostream>
#include <fstream>
#include <string.h>
#include <arm_neon.h>

int32_t NowMicros() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return static_cast<int32_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

using namespace std;
int glob = 0;

float FaceVerify(const float* feature1, const float* feature2) {
  float simi = 0;
 
  for (int i = 0; i <512; ++i) {
    simi += feature1[i] * feature2[i];
  }
  return (simi + 1.0) / 2.0 ;
}

float neon_FaceVerify(const float* feature1, const float* feature2,float* alsum) {
  float simi = 0;
  
  float *sum = alsum;
  int size = 512;
  const float* w = feature1;
  const float* m = feature2;
  
  int nn = size >> 3;
   asm volatile(
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
	ifstream infile("./feature_1.bin",std::ios::binary|std::ios::in);
  float in_1[512*100];
  while(infile.read((char*)in_1,512*100*sizeof(float))) {

  }
	infile.close();
	
	infile.open("./feature_2.bin",std::ios::binary|std::ios::in);
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
	for(int i=0;i<4;i++) {
		cout<<result[i]<<endl;
	}
	
	
  float *sum =  (float*)memalign(16,4*sizeof(float));
	
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
		for(int i=0;i<100;i++) {
			score = FaceVerify(temp, temp_1);
		}
	 end = NowMicros();
	 delete[] temp;
	 delete[] temp_1;
	 all = all+(end-start);
	}
	
	 		
	cout<<glob<<endl;
	
	printf("%fms\n",all/1000.0);
	printf("%f\n",score);
	
	float *w = (float*)memalign(16,512*sizeof(float));
 	  float *m = (float*)memalign(16,512*sizeof(float));
     memcpy(w, in_1, sizeof(float)*512);
	  memcpy(m, in_2, sizeof(float)*512);
 
  	
	  start = NowMicros();
  	for(int i=0;i<100;i++) {
  			score = neon_FaceVerify(w, m, sum);
  	}
  	end = NowMicros();
  	
  	all = all+(end-start);
  
	
    
	
	printf("%fms\n",all/1000.0);
	printf("%f\n",score);
	return 0;
}
