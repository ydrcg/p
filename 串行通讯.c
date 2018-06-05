#include <at89x51.h>
#include <intrins.h>

/*************************************************功能3所需变量********************************************************/
unsigned char ledCondition=0;										//控制8路LED灯显示效果
/**********************************************功能4和功能5所需变量****************************************************/
unsigned char PWM_brightnessControl_count=0;						//记录中断次数
/*************************************************功能6所需变量********************************************************/
bit direc_flag=0;             										//占空比更新方向
double PWM_breathingLamp_count=0.0; 								//记录中断次数
double PWM_VLAUE=5.0;    											//占空比比对值
unsigned int HUXI_COUNT=0;    										//占空比更新时间(记录中断次数)
/*************************************************功能7所需变量********************************************************/
unsigned char flag=0;
unsigned char temp=0;												//临时变量
unsigned char comdata=0;
unsigned char temperature_data_H=0,temperature_data_L=0;			//温度高8位与温度低8位
unsigned char temperature_data_H_temp=0,temperature_data_L_temp=0;	//校验用的温度高8位与温度低8位
unsigned char humidity_data_H=0,humidity_data_L=0;					//湿度高8位与湿度低8位
unsigned char humidity_data_H_temp=0,humidity_data_L_temp=0;		//校验用的湿度高8位与湿度低8位
unsigned char check_data_temp=0;									//校验用的校验8位
/***********************************************串行口通信所需变量*****************************************************/
unsigned char receiveData;											//存放串口接收到的数据(即模块识别后的返回值)

//功能：延时10微秒
void  Delay_10us(void)
{
	unsigned char i;
	i--;
	i--;
	i--;
	i--;
	i--;
	i--;
}

//功能：延时0.1毫秒
void Delay(unsigned int j)
{      
	unsigned char i;
	for(;j>0;j--)
	{ 	
		for(i=0;i<27;i++);
	}
}
				 
//串口发送函数
void sendCharData(unsigned char Data)
{
	while(1)
	{
		if((RI==0)&&(TI==0))
		{
			SBUF=Data;
			break;
		}
   }
}

//定时器T1中断初始化函数
void timer1_init(void)
{		
				                        			//定时器T1为工作方式2(自动重载初值的8位定时器)
	TH1=0xfd;
	TL1=0xfd;										//波特率9600bps(11.0592Mhz晶振)
	TR1=1;											//启动定时器T1,即启动波特率发生器					
	SCON=0x50;										//定义串行口工作方式1,波特率9600bps,允许串口接收
//	SM0=0;											//定义串行口工作方式1
//	SM1=1;
//	REN=1; 											//允许串口接收
	EA=1;  											//开总中断
	ES=1;  											//开串口中断
}

//功能1：点亮LED灯 和 功能2：熄灭LED灯
void controlLed()
{	
	while(receiveData==0x01)						//识别到人喊“开灯“,语音识别模块会从串口输出1,点亮LED灯
	{		  
		P0=0x00; 									//点亮全部LED灯
	}
	while(receiveData==0x02)						//识别到人喊“关灯“,语音识别模块会从串口输出2,熄灭LED灯
	{			   
		P0=0xff;									//熄灭全部LED灯
	}
	P0=0xff;										//指令变化时,熄灭全部LED灯	
}

//功能3：流水灯
void waterLamp(void)
{	
	unsigned char i;								   	
	while(receiveData==0x03)
	{				  
		ledCondition=0x7F;							//LED灯初始化显示效果为 0111 1111
		for(i=0;i<8;i++)
		{
			P0=ledCondition;
			Delay(1000);	  						//延时0.1s	  
			ledCondition=_cror_(ledCondition,1); 	//循环右移 效果：0111 1111 -> 1011 1111
			if(receiveData!=0x03)
			{
				P0=0xff;							//指令变化时,熄灭全部LED灯
				break;									
			}	
		}						  
	}
}			

//功能4：LED灯较低亮度显示(一档亮度) 和 功能5：LED灯较高亮度显示(二档亮度)
void brightnessControl(void)
{
	TMOD=0x21;										//维持定时器T1为工作方式1,设置定时器T0为工作方式1(M1=0,M0=1)
	if(receiveData==0x04)
	{										
		TH0=0xff;									//高八位为(65536-10)/256
		TL0=0xf7;									//低八位为(65536-10)%256,即初值为10个机器周期,定时约10.850684us
		EA=1;										//开总中断
		ET0=1;										//开定时器T0中断
		TR0=1;										//启动定时器T0
		while(receiveData==0x04);
		TR0=0;										//关闭定时器T0
		PWM_brightnessControl_count=0;				//PWM_brightnessControl_count清零
		P0=0xff;									//熄灭LED灯
	}
	if(receiveData==0x05)
	{
		TH0=0xff;									//高八位为(65536-10)/256
		TL0=0xf7;									//低八位为(65536-10)%256,即初值为10个机器周期,定时约10.850684us
		EA=1;										//开总中断
		ET0=1;										//开定时器T0中断
		TR0=1;										//启动定时器T0
		while(receiveData==0x05);					
		TR0=0;										//关闭定时器T0
		PWM_brightnessControl_count=0;				//PWM_brightnessControl_count清零
		P0=0xff;									//熄灭LED灯
	}
}

//功能6：呼吸灯
void breathingLamp(void)
{
	TMOD=0x22;										//维持定时器T1为工作方式1,设置定时器T0为工作方式2(M1=1,M0=0)
	TH0=0x47;               						//定时器溢出值设置,每隔200us发起一次中断
	TL0=0X47;
  	TR0=1;                  						//定时器0开始计时
 	ET0=1;                  						//开定时器0中断
 	EA=1;                       					//开总中断
	while(receiveData==0x06);				
	TR0=0;											//关闭定时器T0
	PWM_breathingLamp_count=0.0;					//计数清零
    HUXI_COUNT=0;
	P0=0xff;										//熄灭LED灯
}

//comdata操作
void COM(void)							
{
	unsigned char i;
    for(i=0;i<8;i++)	   
	{
		flag=2;	
	   	while((!P2_0)&&flag++);
		Delay_10us();
		Delay_10us();
		Delay_10us();
	  	temp=0;
	    if(P2_0)temp=1;
		flag=2;
		while((P2_0)&&flag++);
	   	//超时则跳出for循环		  
	   	if(flag==1)break;
	   	//判断数据位是0还是1	 	   	   
		// 如果高电平高过预定0高电平值则数据位为 1 	   	 
		comdata<<=1;
	   	comdata|=temp;        //0
	}
}

//功能7：读取并发送温湿度模块数据	
void getTemperatureHumidity(void)
{
	while(receiveData==0x07)
	{
		//主机拉低18ms 
	    P2_0=0;
		Delay(180);
		P2_0=1;
		//总线由上拉电阻拉高 主机延时20us
		Delay_10us();
		Delay_10us();
		Delay_10us();
		Delay_10us();
		//主机设为输入 判断从机响应信号 
		P2_0=1;
		//判断从机是否有低电平响应信号 如不响应则跳出，响应则向下运行	  
		if(!P2_0)		 //T !	  
		{
			flag=2;
			//判断从机是否发出 80us 的低电平响应信号是否结束	 
			while((!P2_0)&&flag++);
			flag=2;
			//判断从机是否发出 80us 的高电平，如发出则进入数据接收状态
			while((P2_0)&&flag++);
			//数据接收状态		 
			COM();
			humidity_data_H_temp=comdata;
			COM();
		 	humidity_data_L_temp=comdata;
			COM();
			temperature_data_H_temp=comdata;
			COM();
			temperature_data_L_temp=comdata;
			COM();
			check_data_temp=comdata;
			P2_0=1;
			//数据校验 
			temp=(temperature_data_H_temp+temperature_data_L_temp+humidity_data_H_temp+humidity_data_L_temp);
			if(temp==check_data_temp)
			{
				temperature_data_H=temperature_data_H_temp;
			   	temperature_data_L=temperature_data_L_temp;
				humidity_data_H=humidity_data_H_temp;
			   	humidity_data_L=humidity_data_L_temp;

				sendCharData(0xEE); 				//发送开始标志数0xEE
		   		Delay(10);							//延时1ms
				sendCharData(temperature_data_H); 	//发送温度高8位(整数部分)
		   		Delay(10);							//延时1ms
				sendCharData(temperature_data_L);	//发送温度低8位(小数部分)
				Delay(10);							//延时1ms
				sendCharData(humidity_data_H);		//发送湿度高8位(整数部分)
				Delay(10);							//延时1ms
				sendCharData(humidity_data_L);		//发送湿度低8位(小数部分)
				Delay(10);							//延时1ms
				sendCharData(0xFF); 				//发送结束标志数0xFF
				Delay(10);							//延时1ms

				receiveData=0x00;					//只读取一次温湿度数据
			}
		}
	}
}

void main(void)
{	
	IP=0x10;										//设置串行口为高优先级中断,定时器T0为低优先级中断
	TMOD=0x21;										//默认定时器T0为工作方式1,定时器T1为工作方式2
	timer1_init();
	while(1)
	{	
		controlLed(); 								//功能1：点亮LED灯 和 功能2：熄灭LED灯
		waterLamp();								//功能3：流水灯
		brightnessControl();						//功能4：LED灯较低亮度显示(一档亮度) 和 功能5：LED灯较高亮度显示(二档亮度)
		breathingLamp(); 							//功能6：呼吸灯
		getTemperatureHumidity();					//功能7：读取并发送温湿度模块数据
	}
}

//串行口中断函数
void serial() interrupt 4 
{
	if(RI==1)										//如果是接收中断,进行下面工作
	{
		receiveData=SBUF;							//串口接收到的数据存放至变量receiveData
		RI=0;										//接收中断标志位清零
		SBUF=receiveData;							
	}
	if(TI==1)										//如果是发送中断,进行下面工作
	{
		TI=0; 										//发送中断标志位清零
	}
}

//定时器T0中断函数
void timer0() interrupt 1
{
	if(receiveData==0x04)							//LED灯较低亮度显示(一档亮度)
	{
		TR0=0;										//赋初值定时,关闭定时器
		TH0=0xff;									//高八位为(65536-10)/256
		TL0=0xf7;									//低八位为(65536-10)%256,即10个机器周期,耗时约为10.850684us
		TR0=1;										//打开定时器T0	
		PWM_brightnessControl_count++;				//每中断1次，PWM_brightnessControl_count加1
		if(PWM_brightnessControl_count>=100)  		//若执行完1轮PWM周期(中断100次，即1000个机器周期)
		  	PWM_brightnessControl_count=0;      	//PWM_brightnessControl_count清零，重新开始新一轮的PWM周期
		if(PWM_brightnessControl_count<=90)   		//在执行PWM周期的前90%的期间内
			P0=0xFF;     							//送高电平,熄灭Led灯,占空比为90%
		else 										//在执行该轮PWM周期余下部分的期间内
			P0=~P0;									//点亮LED灯
	}
	if(receiveData==0x05)	   						//LED灯较高亮度显示(二档亮度)
	{
		TR0=0;										//赋初值定时,关闭定时器
		TH0=0xff;									//高八位为(65536-10)/256
		TL0=0xf7;									//低八位为(65536-10)%256,即10个机器周期,耗时约为10.850684us
		TR0=1;										//打开定时器T0	
		PWM_brightnessControl_count++;				//每中断1次，PWM_brightnessControl_count加1
		if(PWM_brightnessControl_count>=100)  		//若执行完1轮PWM周期(中断100次，即1000个机器周期)
		  	PWM_brightnessControl_count=0;      	//PWM_brightnessControl_count清零，重新开始新一轮的PWM周期
		if(PWM_brightnessControl_count<=60)   		//在执行PWM周期的前60%的期间内
			P0=0xFF;     							//送高电平,熄灭Led灯,占空比为60%
		else 										//在执行该轮PWM周期余下部分的期间内
			P0=~P0;									//点亮LED灯
	}
	if(receiveData==0x06)			   				//呼吸灯
	{
		PWM_breathingLamp_count+=0.25;
    	HUXI_COUNT++;								//定时器T0每中断1次,HUXI_COUNT就加1
										
    	if(PWM_breathingLamp_count==PWM_VLAUE)  	//判断是否到了点亮LED的时候
    		P0=0x00;                  				//点亮LED灯
	    if(PWM_breathingLamp_count==10.0)      	 	//当前周期结束
	    {
	        P0=0xff;                    			//熄灭LED灯
	        PWM_breathingLamp_count=0.0;        	//重新计时
	    }

	    if((HUXI_COUNT==100)&&(direc_flag==0))		//定时器T0每中断100次且direc_flag=0,占空比就更新一次
	    {                               			
	        HUXI_COUNT=0.0;							//占空比更新时间清零
	        PWM_VLAUE+=0.25;						//占空比增加2.5%
	        if(PWM_VLAUE==9.75)          			//若占空比为97.5%
	            direc_flag=~direc_flag; 			//占空比更改方向
	    }
	    if((HUXI_COUNT==100)&&(direc_flag==1))		//定时器T0每中断100次且direc_flag=1,占空比就更新一次
	    {                               			
	        HUXI_COUNT=0.0;							//占空比更新时间清零
	        PWM_VLAUE-=0.25;						//占空比减少2.5%
	        if(PWM_VLAUE==0.25)          			//若占空比为2.5%
	            direc_flag=~direc_flag; 			//占空比更改方向
	    }							
	}			
}	 		  