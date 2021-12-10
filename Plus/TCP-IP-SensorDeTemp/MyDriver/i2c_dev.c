/*************************************************************************************************/
/**
  @brief Headers
**/
/*************************************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/uaccess.h>
#include <linux/pinctrl/consumer.h>
#include<linux/workqueue.h>

#define DEVICE_NAME "prueba-td3"
#define BASE_MINOR 0
#define MINOR_COUNT 1
#define DEVICE_CLASS_NAME "prueba-td3"
#define DEVICE_PARENT NULL

#define BYTE2READ 30*4

#define BMP280_SLAVE_DIRR_TX 0x76
#define BMP280_id 0xD0
#define BMP280_ctrl_meas 0xF4
#define BMP280_config 0xF5
#define BMP280_start2read 0xF7
#define BMP280_config_calib00 0x88

#define Myconfig_BMP280_ctrl_meas 0x27
#define Myconfig_BMP280_config 0x80

#define CONTROL_MODULE_UART1_CTSN 0x978 // funcion 3 -> I2C2_SDA
#define CONTROL_MODULE_UART1_RTSN 0x97C // funcion 3 -> I2C2_SCL
#define CONTROL_MODULE_UART1_CTSN_FUNCTION3 0x3
#define CONTROL_MODULE_UART1_CTSN_PULLUP_FASTSLEWRATE (0x3 << 4)

#define CM_PER 0x44E00000
#define CM_PER_OFFSET 0x400
#define CM_PER_I2C_2 0x44
#define CM_PER_L4LS_CLKSTCTRL 0x00
#define CM_PER_I2C2_CLKCTRL_MODULEMODE  (0x2 << 0) // Module is explicitly enabled
#define CM_PER_I2C2_CLKCTRL_IDLEST  (0x0 << 16) // Module is fully functional

#define CONTROL_MODULE 0x44E10000
#define CONTROL_MODULE_OFFSET 0x2000

#define I2C2 0x4819C000
#define I2C2_OFFSET 0x1000

#define I2C_2_CON   (uint32_t) 0xA4
#define I2C_2_PSC   (uint32_t) 0xB0
#define I2C_2_SCLL  (uint32_t) 0xB4
#define I2C_2_SCLH  (uint32_t) 0xB8
#define I2C_2_OA    (uint32_t) 0xA8
#define I2C_2_SYSC  (uint32_t) 0x10
#define I2C_2_SA    (uint32_t) 0xAC
#define I2C_2_DATA 0x9C
#define I2C_2_CNT 0x98
#define I2C_2_BUF 0x94
#define I2C_2_SYSS 0x90
#define I2C_2_IRQSTATUS 0x28
#define I2C_2_IRQENABLE_CLR 0x30
#define I2C_2_IRQSTATUS_RAW 0x24
#define I2C_2_IRQENABLE_SET 0x2C

#define I2C_2_IRQSTATUS_ARDY (1 << 2)
#define I2C_2_IRQSTATUS_RRDY (1 << 3)
#define I2C_2_IRQSTATUS_XRDY (1 << 4)
//#define I2C_2_IRQSTATUS_STC (1 << 6)
#define I2C_2_IRQSTATUS_BF (1 << 8)
#define I2C_2_IRQSTATUS_BB (1 << 12)
#define I2C_2_IRQSTATUS_XUDF (1 << 10)
#define I2C_2_IRQSTATUS_ROVR (1 << 11)


#define I2C_2_CON_RESET (0 << 15)
#define I2C_2_SETFREQ 100000

/*************************************************************************************************/
/*************************************************************************************************/

/*************************************************************************************************/
/*************************************** Estructuras *********************************************/
/**
    @brief Voy a mantener el estado del driver en esta estructura llamada "state"
**/
/*************************************************************************************************/
/* Per-device (per-bank) structure */
static struct state {
  dev_t devtype;          // numero mayor y menor
  struct class *devclass; // class_create()
  struct device *dev;     // device_create()
  struct cdev chardev;    // cdev_add()

  unsigned int freq;
  int irq;
  int write;
  void __iomem *_reg_cm_per;
  void __iomem *_reg_cm;
  void __iomem *_reg_i2c2;

} state;

static struct state state;

struct bmp280_dev
{
  uint8_t initialized;
  uint8_t chip_id;
  int TX_count;
  int RX_count;
  int *TX_buff;
  int *RX_buff;
  unsigned short dig_T1_0, dig_T1_1;
  signed short dig_T2_0, dig_T2_1, dig_T3_0, dig_T3_1;
  unsigned short dig_P1_0, dig_P1_1;
  signed short dig_P2_0, dig_P2_1, dig_P3_0, dig_P3_1, dig_P4_0, dig_P4_1, dig_P5_0, dig_P5_1, dig_P6_0, dig_P6_1;
  signed short dig_P7_0, dig_P7_1, dig_P8_0, dig_P8_1, dig_P9_0, dig_P9_1;
  long t_fine;
};

//struct bmp280_dev bmp280_dev = {0, 0, 0, 0, 0, {NULL}, {NULL}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
struct bmp280_dev bmp280_dev;
static DECLARE_WAIT_QUEUE_HEAD(my_wq);
volatile static int condition = 0;
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/************************ Estructuras platform_driver y of_device_id *****************************/
/**
    @brief Estructuras platform_driver y of_device_id
**/
/*************************************************************************************************/
static const struct of_device_id driver_of_match[] = {
{ .compatible = "test-td3", },
{ }
};

//MODULE_DEVICE_TABLE(of, driver_of_match);

static int driver_probe (struct platform_device *pdev);
static int driver_remove (struct platform_device *pdev);

static struct platform_driver mypdriver = {
  .probe      = driver_probe,				                              
  .remove     = driver_remove, 
  .driver = {
    .name = "test-td3",	
    .owner = THIS_MODULE,
    .of_match_table = of_match_ptr(driver_of_match),
  },                             
};
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/**
  @brief Funcion para setear registros

  @param base
  @param offset
  @param mask
  @param value
  @param pdev

  @returns void
**/
/*************************************************************************************************/
void set_registers (volatile void *base, uint32_t offset, uint32_t mask, uint32_t value)
{
  uint32_t old_value = ioread32 (base + offset);  // Leo los datos del registro
  //pr_info(DEVICE_NAME": set_registers: Lei:  %#x\n", old_value);
  old_value = old_value & ~(mask);  // Borro los datos que voy a escribir
  value = value & mask;   // Me quedo con los valores que van
  value = value | old_value;
  //pr_info(DEVICE_NAME": set_registers: Escribo el registro con:  %#x\n", value);
  iowrite32(value, base + offset);
  return;
}
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/**
  @brief Funcion para setear registros

  @param base
  @param offset
  @param mask
  @param value
  @param pdev

  @returns void
**/
/*************************************************************************************************/
volatile uint32_t get_register (uint32_t *reg, uint32_t offset)
{
  uint32_t out=0;

  out=ioread32(reg + offset);
  pr_info(DEVICE_NAME": get_register : Lei el registro:  %d\n", out);

  return out;
}
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/**
  @brief Funcion para leer y escribir al BMP280

  @param void

  @returns int
**/
/*************************************************************************************************/
int bmp280_read (void)
{
  uint32_t aux=0, count=0;
  int event;

  //pr_info(DEVICE_NAME": Limpio las IRQs\n");
  set_registers(state._reg_i2c2, I2C_2_IRQSTATUS, (uint32_t)(I2C_2_IRQSTATUS_RRDY|I2C_2_IRQSTATUS_ARDY|I2C_2_IRQSTATUS_XRDY) , (uint32_t)(I2C_2_IRQSTATUS_RRDY|I2C_2_IRQSTATUS_ARDY|I2C_2_IRQSTATUS_XRDY));  
  //pr_info(DEVICE_NAME": Deshabilito las IRQs\n");
  set_registers(state._reg_i2c2, I2C_2_IRQENABLE_CLR, (uint32_t)(I2C_2_IRQSTATUS_RRDY|I2C_2_IRQSTATUS_ARDY|I2C_2_IRQSTATUS_XRDY) , (uint32_t)(I2C_2_IRQSTATUS_RRDY|I2C_2_IRQSTATUS_ARDY|I2C_2_IRQSTATUS_XRDY));  

  //pr_info(DEVICE_NAME": Lo seteo como master y TX\n");
  set_registers(state._reg_i2c2, I2C_2_CON, (uint32_t)0x0000600, (uint32_t)0x000600);

  //pr_info(DEVICE_NAME": Habilito las IRQs\n");
  set_registers(state._reg_i2c2, I2C_2_IRQENABLE_SET, (uint32_t)(I2C_2_IRQSTATUS_ARDY|I2C_2_IRQSTATUS_XRDY) , (uint32_t)(I2C_2_IRQSTATUS_ARDY|I2C_2_IRQSTATUS_XRDY));  
      
  //pr_info(DEVICE_NAME": Direccion del esclavo\n");
  set_registers(state._reg_i2c2, I2C_2_SA, (uint32_t)0x000003FF , (uint32_t)BMP280_SLAVE_DIRR_TX);   

  //pr_info(DEVICE_NAME": Pongo en el registro I2C_CNT la cantidad de bytes para TX\n");
  set_registers(state._reg_i2c2, I2C_2_CNT, (uint32_t)0x000000FF , (uint32_t)bmp280_dev.TX_count);

	aux = ioread32(state._reg_i2c2 + I2C_2_IRQSTATUS_RAW);
	count= 0;
	while(aux & I2C_2_IRQSTATUS_BB)
  {
    pr_info(DEVICE_NAME": Verifico que el bus este libre\n");
		msleep(1); 
		aux = ioread32(state._reg_i2c2 + I2C_2_IRQSTATUS_RAW);
		count++;
		if(count >= 10)
    {
			pr_info(DEVICE_NAME": Paso mucho tiempo esperando que el bus se libere\n");
			return -1;
		}
	}
  pr_info(DEVICE_NAME": Bus liberado\n");

  pr_info(DEVICE_NAME": START TX\n");
  set_registers(state._reg_i2c2, I2C_2_CON, (uint32_t)0x0000003, (uint32_t)0x000003);

  pr_info(DEVICE_NAME": Duermo!\n");
  event=wait_event_interruptible(my_wq, condition != 0);
  pr_info(DEVICE_NAME": Desperte!\n");
  condition=0;

  mdelay(100);

  //pr_info(DEVICE_NAME": Lo seteo como master y RX\n");
  set_registers(state._reg_i2c2, I2C_2_CON, (uint32_t)0x0000600, (uint32_t)0x00400); 

  //pr_info(DEVICE_NAME": Pongo en el registro I2C_CNT la cantidad de bytes para RX\n");
  set_registers(state._reg_i2c2, I2C_2_CNT, (uint32_t)0x000000FF , (uint32_t)bmp280_dev.RX_count);

	aux = ioread32(state._reg_i2c2 + I2C_2_IRQSTATUS_RAW);
	count= 0;
	while(aux & I2C_2_IRQSTATUS_BB)
  {
    pr_info(DEVICE_NAME": Verifico que el bus este libre\n");
		msleep(1); 
		aux = ioread32(state._reg_i2c2 + I2C_2_IRQSTATUS_RAW);
		count++;
		if(count >= 10)
    {
			pr_info(DEVICE_NAME": Paso mucho tiempo esperando que el bus se libere\n");
			return -1;
		}
	}
  pr_info(DEVICE_NAME": Bus liberado\n"); 

  //pr_info(DEVICE_NAME": Lo saco del reset\n");
  set_registers(state._reg_i2c2, I2C_2_CON, (uint32_t)0x0008000, (uint32_t)0x08000); 

  //pr_info(DEVICE_NAME": Limpio las IRQs\n");
  set_registers(state._reg_i2c2, I2C_2_IRQSTATUS, (uint32_t) 0xFFFFFFFF, (uint32_t)0xFFFFFFFF);
  //pr_info(DEVICE_NAME": Deshabilito las IRQs\n");
  set_registers(state._reg_i2c2, I2C_2_IRQENABLE_CLR, (uint32_t) 0xFFFFFFFF, (uint32_t)0xFFFFFFFF);

  //pr_info(DEVICE_NAME": Habilito las IRQs de RX\n");
  set_registers(state._reg_i2c2, I2C_2_IRQENABLE_SET, (uint32_t)(I2C_2_IRQSTATUS_RRDY) , (uint32_t)(I2C_2_IRQSTATUS_RRDY)); 

  pr_info(DEVICE_NAME": START RX\n");
  set_registers(state._reg_i2c2, I2C_2_CON, (uint32_t)0x0000003, (uint32_t)0x000003);

  pr_info(DEVICE_NAME": Duermo!\n");
  wait_event_interruptible(my_wq, condition != 0);
  condition=0;
  pr_info(DEVICE_NAME": Desperte!\n");

  return 0;
}

void save_calib_param(void)
{
  bmp280_dev.dig_T1_0 = bmp280_dev.RX_buff[0];
  bmp280_dev.dig_T1_1 = bmp280_dev.RX_buff[1];
  bmp280_dev.dig_T2_0 = bmp280_dev.RX_buff[2];
  bmp280_dev.dig_T2_1 = bmp280_dev.RX_buff[3];
  bmp280_dev.dig_T3_0 = bmp280_dev.RX_buff[4];
  bmp280_dev.dig_T3_1 = bmp280_dev.RX_buff[5];
  bmp280_dev.dig_P1_0 = bmp280_dev.RX_buff[6];
  bmp280_dev.dig_P1_1 = bmp280_dev.RX_buff[7];
  bmp280_dev.dig_P2_0 = bmp280_dev.RX_buff[8];
  bmp280_dev.dig_P2_1 = bmp280_dev.RX_buff[9];
  bmp280_dev.dig_P3_0 = bmp280_dev.RX_buff[10];
  bmp280_dev.dig_P3_1 = bmp280_dev.RX_buff[11];
  bmp280_dev.dig_P4_0 = bmp280_dev.RX_buff[12];
  bmp280_dev.dig_P4_1 = bmp280_dev.RX_buff[13];
  bmp280_dev.dig_P5_0 = bmp280_dev.RX_buff[14];
  bmp280_dev.dig_P5_1 = bmp280_dev.RX_buff[15];
  bmp280_dev.dig_P6_0 = bmp280_dev.RX_buff[16];
  bmp280_dev.dig_P6_1 = bmp280_dev.RX_buff[17];
  bmp280_dev.dig_P7_0 = bmp280_dev.RX_buff[18];
  bmp280_dev.dig_P7_1 = bmp280_dev.RX_buff[19];
  bmp280_dev.dig_P8_0 = bmp280_dev.RX_buff[20];
  bmp280_dev.dig_P8_1 = bmp280_dev.RX_buff[21];
  bmp280_dev.dig_P9_0 = bmp280_dev.RX_buff[22];
  bmp280_dev.dig_P9_1 = bmp280_dev.RX_buff[23];
  return;
}

static int bmp280_init(void)
{
  pr_info(DEVICE_NAME": Voy a leer el chip ID\n");    /////////////////////
  bmp280_dev.TX_count=1;
  bmp280_dev.TX_buff[0]=BMP280_id;
  bmp280_dev.RX_count=1;
  if(bmp280_read() == -1)
  {
    pr_info(DEVICE_NAME": bmp280_read: Fallo!!\n");
    return -1;
  }
  pr_info(DEVICE_NAME": El chip ID es: %#x\n", bmp280_dev.RX_buff[0]);    

  pr_info(DEVICE_NAME": Leo los parametros de calibracion\n");     
  bmp280_dev.TX_count=1;
  bmp280_dev.TX_buff[0]=BMP280_config_calib00;
  bmp280_dev.RX_count=24;
  if(bmp280_read() == -1)
  {
    pr_info(DEVICE_NAME": bmp280_read: Fallo!!\n");
    return -1;
  }
  save_calib_param();

  pr_info(DEVICE_NAME": Configuracion de los registros\n");    
  bmp280_dev.TX_count=5;
  bmp280_dev.TX_buff[0]=BMP280_ctrl_meas;
  bmp280_dev.TX_buff[1]=Myconfig_BMP280_ctrl_meas;   // Oversampling x1 temp y pres y modo normal
  bmp280_dev.TX_buff[2]=BMP280_config;
  bmp280_dev.TX_buff[3]=Myconfig_BMP280_config;   // tstandly 1000 ms
  bmp280_dev.TX_buff[4]=BMP280_ctrl_meas;   // Leo los registros para verificar que se escribieron
  bmp280_dev.RX_count=2;
  if(bmp280_read() == -1)
  {
    pr_info(DEVICE_NAME": bmp280_read: Fallo!!\n");
    return -1;
  }
  else 
  {
    pr_info(DEVICE_NAME": bmp280_read: bmp280_dev.RX_buff[0] es %#X\n", bmp280_dev.RX_buff[0]);
    pr_info(DEVICE_NAME": bmp280_read: bmp280_dev.RX_buff[1] es %#X\n", bmp280_dev.RX_buff[1]);
    if(bmp280_dev.RX_buff[0] != Myconfig_BMP280_ctrl_meas){
      pr_info(DEVICE_NAME": bmp280_read: Inicializacion incorrecta!!\n");
      return -1;
    }
    if(bmp280_dev.RX_buff[1] != Myconfig_BMP280_config){
      pr_info(DEVICE_NAME": bmp280_read: Inicializacion incorrecta!!\n");
      return -1;
    }
  }
  return 0;   // Ya esta inicializado
}
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/**
  @brief Handler de interrupciones

  @param irq
  @param devid

  @returns 
**/
/*************************************************************************************************/
irqreturn_t driver_isr(int irq, void *devid, struct pt_regs *ptregs) 
{
  uint32_t irqstate;
  static int aux=0; 
  static int aux2=0;

  irqstate=ioread32(state._reg_i2c2 + I2C_2_IRQSTATUS);
  //set_registers(state._reg_i2c2, I2C_2_IRQSTATUS, (uint32_t) 0xFFFFFFFF, 0xFFFFFFFF);
  iowrite32((uint32_t)0xFFFFFFFF ,state._reg_i2c2 + I2C_2_IRQSTATUS);

  //pr_info(DEVICE_NAME": Deshabilito las IRQs\n");
  //set_registers(state._reg_i2c2, I2C_2_IRQENABLE_CLR, (uint32_t) 0xFFFFFFFF, (uint32_t)0xFFFFFFFF);
 
  //pr_info(DEVICE_NAME": ISR: Entre al manejador de interrupciones irqstate es: %#x\n", irqstate);

  if (( (irqstate & I2C_2_IRQSTATUS_XRDY)>> 4) == 0x1 ) 
  {
    pr_info(DEVICE_NAME": ISR: XRDY interrup\n");
    set_registers(state._reg_i2c2, I2C_2_DATA, (uint32_t)0x000000FF , bmp280_dev.TX_buff[aux]);
    bmp280_dev.TX_count--;
    aux++;
    //pr_info(DEVICE_NAME": ISR: Borro los estados de interrupciones\n");
    set_registers(state._reg_i2c2, I2C_2_IRQSTATUS, (uint32_t)0x0000003E , 0x3E);

    if(!bmp280_dev.TX_count)
    {
      pr_info(DEVICE_NAME": ISR: Termine de transmitir, desactivo TX IRQ\n");
      set_registers(state._reg_i2c2, I2C_2_IRQENABLE_CLR, (uint32_t)I2C_2_IRQSTATUS_XRDY , (uint32_t)I2C_2_IRQSTATUS_XRDY);
      set_registers(state._reg_i2c2, I2C_2_IRQENABLE_SET, (uint32_t)(I2C_2_IRQSTATUS_ARDY) , (uint32_t)(I2C_2_IRQSTATUS_ARDY));
      aux=0;   
    }
    
    return IRQ_HANDLED;
  }
  
  if ( ((irqstate & I2C_2_IRQSTATUS_RRDY)  >> 3) == 0x1)
  {
    pr_info(DEVICE_NAME": ISR: RRDY interrup\n");
    //pr_info(DEVICE_NAME": ISR: Borro los estados de interrupciones\n");
    bmp280_dev.RX_buff[aux2]=ioread32(state._reg_i2c2 + I2C_2_DATA);
    pr_info(DEVICE_NAME": ISR: Lei del sensor: %#x\n", bmp280_dev.RX_buff[aux2]);
    aux2++;
    bmp280_dev.RX_count--;
    set_registers(state._reg_i2c2, I2C_2_IRQSTATUS, (uint32_t)0x0000003E , 0x3E);
    if(!bmp280_dev.RX_count)
    {
      pr_info(DEVICE_NAME": ISR: Termine de recibir, desactivo RX IRQ\n");
      set_registers(state._reg_i2c2, I2C_2_IRQENABLE_CLR, (uint32_t)I2C_2_IRQSTATUS_RRDY , (uint32_t)I2C_2_IRQSTATUS_RRDY);
      set_registers(state._reg_i2c2, I2C_2_IRQENABLE_SET, (uint32_t)(I2C_2_IRQSTATUS_ARDY) , (uint32_t)(I2C_2_IRQSTATUS_ARDY));  
      aux2=0;
    }
    return IRQ_HANDLED;
  }

  if ( ((irqstate & I2C_2_IRQSTATUS_ARDY) >> 2) == 0x1)
  {
    pr_info(DEVICE_NAME": ISR: ARDY interrup\n");
    //pr_info(DEVICE_NAME": ISR: Borro los estados de interrupciones\n");
    set_registers(state._reg_i2c2, I2C_2_IRQSTATUS, (uint32_t)0x0000003E , 0x3E);
    condition=10;
    wake_up_interruptible(&my_wq);
    return IRQ_NONE;
  }

  pr_info(DEVICE_NAME": ISR: Interrupcion desconocida!!!!\n");
  return IRQ_HANDLED;
}
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/**
  @brief Funcion que se ejecutara con el uso de la syscall open()

  @param inode
  @param file

  @returns 0: sin error, -1: error
**/
/*************************************************************************************************/

static int driver_open(struct inode *inode, struct file *file) 
{
  //unsigned int maj = imajor(inode);
  unsigned int min = iminor(inode);
  struct state *state_devp;

  pr_info(DEVICE_NAME": Hola! Entre a open! \n");

  /* Consigue la estructura per-device que contiene cdev */
  state_devp = container_of(inode->i_cdev, struct state , chardev);

  /* Facilita el acceso a state_devp desde el resto de los ptos de entrada */
  file->private_data = state_devp;

  if (min < 0)
  {
    pr_err (DEVICE_NAME": Device not found\n");
    return -ENODEV; /* No such device */
  }

  /* Inicializo algunos campos de state */
  //state_devp->freq=100000;

  /* Seteo los pines de la BBB */
  pr_info(DEVICE_NAME": Configuro los pines para I2C_2 \n");
  set_registers(state._reg_cm, CONTROL_MODULE_UART1_CTSN, (uint32_t)0x00000007F , ( 0x3B ));
  set_registers(state._reg_cm, CONTROL_MODULE_UART1_RTSN, (uint32_t)0x00000007F , ( 0x3B ));

  /* Inicializo algunos registros del i2c */
  pr_info(DEVICE_NAME": Inicializo algunos registros del i2c \n");
  
  // Soft Reset
  pr_info(DEVICE_NAME": Realizo un reset \n");
  set_registers(state._reg_i2c2, I2C_2_CON, (uint32_t)(1<<15) , (1<<15)); 

  while ( (get_register(state._reg_i2c2, I2C_2_SYSC) >> 1) != 0 )  // Espero SoftReset
  {
    pr_info(DEVICE_NAME": Esperando el reset...\n");
    mdelay(1);
  }
  pr_info(DEVICE_NAME": Reset completado \n");
  
  //I2C_2_CON   //  No start condition, No stop condition, 7bit address,        
  //set_registers(state._reg_i2c2, I2C_2_CON, (uint32_t)0x0000BFF3 , (uint32_t)0x00);       // check
  //I2C_2_PSC   // PSC +1 = 48Mhz/(3+1) = 12Mhz
  set_registers(state._reg_i2c2, I2C_2_PSC, (uint32_t)0x000000FF , (uint32_t)0x03);       // check
  //I2C_2_SCLL  // 90*iclk = 90*(1/12Mhz) = 7.5 useg
  set_registers(state._reg_i2c2, I2C_2_SCLL, (uint32_t)0x000000FF , (uint32_t)0x53);      // check
  //I2C_2_SCLH  // 90*iclk = 90*(1/12Mhz) = 7.5 useg
  set_registers(state._reg_i2c2, I2C_2_SCLH, (uint32_t)0x000000FF , (uint32_t)0x55);      // check
  //I2C_2_OA    // Direccion propia
  set_registers(state._reg_i2c2, I2C_2_OA, (uint32_t)0x000003FF , (uint32_t)0x36);        // check
  //I2C_2_SYSC
  set_registers(state._reg_i2c2, I2C_2_SYSC, (uint32_t)0x31C , (uint32_t)0x00);       // check
  //Lo saco del reset
  set_registers(state._reg_i2c2, I2C_2_CON, (uint32_t)(1<<15), (uint32_t)(1<<15));

  pr_info(DEVICE_NAME": Registros del i2c inicializados \n");

  pr_info(DEVICE_NAME": Consigo memoria para el buffer de recepcion y de transmision\n");

  if ((bmp280_dev.RX_buff = (int *) kmalloc(BYTE2READ, GFP_KERNEL)) == NULL)
  {
    pr_err (DEVICE_NAME": Insuficiente memoria\n");
    return -ENODEV; /* No such device */
  }

  if ((bmp280_dev.TX_buff = (int *) kmalloc(BYTE2READ, GFP_KERNEL)) == NULL)
  {
    pr_err (DEVICE_NAME": Insuficiente memoria\n");
    return -ENODEV; /* No such device */
  }

  pr_info(DEVICE_NAME": Inicializo el modulo\n"); 
  bmp280_init();

  pr_info(DEVICE_NAME": Funcion open terminada con exito!\n");
  return 0;
}
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/**
  @brief Funcion que se ejecutara con el uso de la syscall read()

  @param file
  @param buff
  @param size
  @param offset

  @returns 0: sin error, -1: error
**/
/*************************************************************************************************/
static ssize_t driver_read(struct file *file, char __user *ubuff, size_t size, loff_t *offset) 
{
  int kbuff[BYTE2READ];

  pr_info(DEVICE_NAME": Entre a driver_read\n"); 

  pr_info(DEVICE_NAME": Verifico si es valido el tamanio del buffer\n");  
  if(size != BYTE2READ)
  {
    pr_info(DEVICE_NAME": El tamanio del buffer no es valido\n");
    return -1;
  }

  pr_info(DEVICE_NAME": Verifico si es valido el buffer\n");  
  if( (access_ok(VERIFY_WRITE, ubuff, size)) == 0)
  {
    pr_info(DEVICE_NAME": access_ok: El buffer es invalido\n");  
    return -1;
  }

  pr_info(DEVICE_NAME": Le pido la temperatura y la presion\n");     
  bmp280_dev.TX_count=1;
  bmp280_dev.TX_buff[0]=BMP280_start2read;
  bmp280_dev.RX_count=6;
  if(bmp280_read() == -1)
  {
    pr_info(DEVICE_NAME": bmp280_read: Fallo!!\n");
    return -1;
  }

  kbuff[0] = bmp280_dev.RX_buff[0];
  kbuff[1] = bmp280_dev.RX_buff[1];
  kbuff[2] = bmp280_dev.RX_buff[2];
  kbuff[3] = bmp280_dev.RX_buff[3];
  kbuff[4] = bmp280_dev.RX_buff[4];
  kbuff[5] = bmp280_dev.RX_buff[5];
  kbuff[6] = bmp280_dev.dig_T1_0;
  kbuff[7] = bmp280_dev.dig_T1_1;
  kbuff[8] = bmp280_dev.dig_T2_0;
  kbuff[9] = bmp280_dev.dig_T2_1;
  kbuff[10] = bmp280_dev.dig_T3_0;
  kbuff[11] = bmp280_dev.dig_T3_1;
  kbuff[12] = bmp280_dev.dig_P1_0;
  kbuff[13] = bmp280_dev.dig_P1_1;
  kbuff[14] = bmp280_dev.dig_P2_0;
  kbuff[15] = bmp280_dev.dig_P2_1;
  kbuff[16] = bmp280_dev.dig_P3_0;
  kbuff[17] = bmp280_dev.dig_P3_1;
  kbuff[18] = bmp280_dev.dig_P4_0;
  kbuff[19] = bmp280_dev.dig_P4_1;
  kbuff[20] = bmp280_dev.dig_P5_0;
  kbuff[21] = bmp280_dev.dig_P5_1;
  kbuff[22] = bmp280_dev.dig_P6_0;
  kbuff[23] = bmp280_dev.dig_P6_1;
  kbuff[24] = bmp280_dev.dig_P7_0;
  kbuff[25] = bmp280_dev.dig_P7_1;
  kbuff[26] = bmp280_dev.dig_P8_0;
  kbuff[27] = bmp280_dev.dig_P8_1;
  kbuff[28] = bmp280_dev.dig_P9_0;
  kbuff[29] = bmp280_dev.dig_P9_1;

  pr_info(DEVICE_NAME": Cargo el buffer del usuario con la informacion\n");
  if(__copy_to_user(ubuff, kbuff, size) != 0)
  {
    pr_info(DEVICE_NAME": bmp280_read: Fallo __copy_to_user\n");
    return -1;
  }

  return 0;
}
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/**
  @brief Funcion que se ejecutara con el uso de la syscall close()

  @param inode
  @param file

  @returns 0: sin error, -1: error
**/
/*************************************************************************************************/
static int driver_release(struct inode *inode, struct file *file) 
{
  pr_err(DEVICE_NAME": driver_release: Entre a driver_release\n");

  kfree(bmp280_dev.TX_buff);
  kfree(bmp280_dev.RX_buff);  

	iowrite32(0x7FF, state._reg_i2c2 + I2C_2_IRQSTATUS); 	// Limpio la int
	iowrite32(0, state._reg_i2c2 + I2C_2_CON);            // Reset

  return 0;
}
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/**
  @brief Funcion que se ejecutara con el uso de la syscall ioctl()

  @param 
  @param 

  @returns 0: sin error, -1: error
**/
/*************************************************************************************************/
static long driver_ioctl(struct file *archivo, unsigned int cmd, unsigned long arg)
{ 
  if (cmd==1) 
	  bmp280_dev.chip_id = (int)arg;

	return 0;
}
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/******************************** Estructura file_operations *************************************/
/**
    @brief Implementacion de las funciones del driver open, read, write, close, ioctl, cada driver
    tiene su propia file_operations
**/

static const struct file_operations driver_file_op = {
  .owner = THIS_MODULE,
  .open = driver_open,
  .read = driver_read,
  .release = driver_release,
  .unlocked_ioctl = driver_ioctl,
};
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/**
  @brief Todos los recursos que utiliza el driver registrados en probe()

  @param pdev Struct platform_device

  @returns Error Handling, predefined errors in the kernel tree
**/
/*************************************************************************************************/

static int driver_probe (struct platform_device *pdev)
{
  int status=0; // request_irq
  int count=0;

  //struct resource *memory = NULL;

  pr_info("%s: Hola! Entre a device probed! \n", pdev->name);

  /* Voy a pedir la interrupcion */
  pr_info("%s: Pido interrupcion al kernel \n", pdev->name);
  if ((state.irq = platform_get_irq(pdev, 0)) < 0)
  {
    pr_err(DEVICE_NAME": platform_get_irq: No se pudo registrar el ISR\n");
    unregister_chrdev_region(state.devtype, MINOR_COUNT);
    cdev_del(&state.chardev);
    class_destroy(state.devclass);          // Destruyo la clase creada en el paso previo
    device_destroy(state.devclass, state.devtype);  // Destruyo el device creado en el paso previo
    platform_driver_unregister(&mypdriver);
    return -EPERM;    // -1
  }
  

  /* Implantamos el ISR */
  pr_info(DEVICE_NAME": Implantamos el ISR \n");
  pr_info(DEVICE_NAME": Numero del IRQ del device: %d\n", state.irq);
  if ((status = request_irq(state.irq, (irq_handler_t)driver_isr, IRQF_TRIGGER_RISING, pdev->name, NULL)) != 0) // Consultar IRQF_NO_SUSPEND
  {
    pr_err(DEVICE_NAME": request_irq: No se pudo registrar el ISR\n");
    unregister_chrdev_region(state.devtype, MINOR_COUNT);
    cdev_del(&state.chardev);
    class_destroy(state.devclass);          // Destruyo la clase creada en el paso previo
    device_destroy(state.devclass, state.devtype);  // Destruyo el device creado en el paso previo
    platform_driver_unregister(&mypdriver);
    return -EPERM;    // -1
  }
  pr_info("%s: ISR implantado \n", pdev->name);
  
  /* Puertos*/
  pr_info("%s: Accedo a la zona del registro del Modulo de control \n", pdev->name);
  if ((state._reg_cm = ioremap(CONTROL_MODULE, CONTROL_MODULE_OFFSET)) == NULL)   
  {
    pr_err(DEVICE_NAME": ioremap: No se pudo acceder al Modulo de control\n");
    unregister_chrdev_region(state.devtype, MINOR_COUNT);
    cdev_del(&state.chardev);
    class_destroy(state.devclass);          // Destruyo la clase creada en el paso previo
    device_destroy(state.devclass, state.devtype);  // Destruyo el device creado en el paso previo
    platform_driver_unregister(&mypdriver);
    free_irq(state.irq, driver_isr);
    return -EPERM;    // -1
  }
  pr_info("%s: Accedi a la zona del registro del Modulo de control \n", pdev->name);

  pr_info("%s: Accedo a la zona del registro I2C_2 \n", pdev->name);
  if ((state._reg_i2c2 = ioremap(I2C2, I2C2_OFFSET)) == NULL)   
  {
    pr_err(DEVICE_NAME": ioremap: No se pudo acceder al registro I2C2\n");
    unregister_chrdev_region(state.devtype, MINOR_COUNT);
    cdev_del(&state.chardev);
    class_destroy(state.devclass);          // Destruyo la clase creada en el paso previo
    device_destroy(state.devclass, state.devtype);  // Destruyo el device creado en el paso previo
    platform_driver_unregister(&mypdriver);
    free_irq(state.irq, driver_isr);
    iounmap(state._reg_cm);
    return -EPERM;    // -1
  }
  pr_info("%s: Accedi a la zona del registro I2C_2 \n", pdev->name);

  pr_info("%s: Accedo a la zona del registro del manejador de reloj \n", pdev->name);
  if ((state._reg_cm_per = ioremap(CM_PER, CM_PER_OFFSET)) == NULL)
  {
    pr_err(DEVICE_NAME": ioremap: No se pudo acceder al CMPER\n");
    unregister_chrdev_region(state.devtype, MINOR_COUNT);
    cdev_del(&state.chardev);
    class_destroy(state.devclass);          // Destruyo la clase creada en el paso previo
    device_destroy(state.devclass, state.devtype);  // Destruyo el device creado en el paso previo
    platform_driver_unregister(&mypdriver);
    free_irq(state.irq, driver_isr);
    iounmap(state._reg_cm);
    iounmap(state._reg_i2c2);
    return -EPERM;    // -1
  }
  pr_info("%s: Accedi a la zona del registro del manejador de reloj \n", pdev->name);

  /* Tengo que habilitar el reloj para el I2C_2 */
  pr_info("%s: Habilito el reloj para el I2C_2 \n", pdev->name);
  //set_registers(state._reg_cm_per, CM_PER_I2C_2, (uint32_t)0x00030003, CM_PER_I2C2_CLKCTRL_MODULEMODE );
  iowrite32(0x02, state._reg_cm_per + CM_PER_I2C_2); // Configuro el clock el I2C

  count=0;
  while(ioread32(state._reg_cm_per + CM_PER_I2C_2) != 2)
  {
    msleep(10);
    count++;
    if (count==10)
    {
      pr_info("%s: Paso mucho tiempo esperando que se habilite el CLK \n", pdev->name);
      return -EPERM;    // -1 
    }
  }
  //set_registers(state._reg_cm_per, CM_PER_L4LS_CLKSTCTRL, (uint32_t)0x1000000, 0x1000000);
  pr_info("%s: Espero un ratito que se habilite \n", pdev->name);
  msleep(10);

  // Otra manera de hacerlo usando los datos del device tree 
  //if ((status = of_property_read_u32(pdev->dev.of_node, "clock-frequency", &state.freq)) != 0) 
  //  state.freq = 100000; // default to 100000 Hz

  pr_info("%s: Probe terminado con exito! \n", pdev->name);

  return 0; 
}
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/**
  @brief 

  @param pdev

  @returns 0: sin error, -1: error
**/
/*************************************************************************************************/
static int driver_remove (struct platform_device *pdev)
{
  pr_info("driver_remove: Remuevo el dispositivo!\n");

  free_irq(state.irq, driver_isr);
  iounmap(state._reg_cm);
  iounmap(state._reg_i2c2);
  iounmap(state._reg_cm_per);

  return 0;
}
/*************************************************************************************************/
/*************************************************************************************************/


static int __init my_pdrv_init (void) 
{
  pr_info("my_pdrv_init: Hola mundo!\n");

  //No es necesario allocar cdev porque lo defini como struct cdev no como struct *cdev !!!
  //pr_info("%s: Obtengo memoria para allocar cdev struct\n", pdev->name);
  //(state.chardev) = cdev_alloc();    // Obtiene memoria para allocar la estructura 

  pr_info("my_pdrv_init: Intento allocar cdev\n");
  if (alloc_chrdev_region(&state.devtype, BASE_MINOR, MINOR_COUNT, DEVICE_NAME) != 0) 
  {
    pr_err(DEVICE_NAME": alloc_chrdev_region: Error no es posible asignar numero mayor\n");
    return -EBUSY;  // Device or resource busy 
  }
  pr_info("my_pdrv_init: Cdev struct allocada\n");


  /* Procedemos a registrar el dispositivo en el sistema */
  pr_info("my_pdrv_init: Procedo a registrar el dispositivo en el sistema\n");

  /* Para inicializarla se puede inicializar a mano o bien una vez definida file_operations...  */
  pr_info("%my_pdrv_init: Cargo cdev struct\n");
  cdev_init(&state.chardev, &driver_file_op);

  /* Para agregarla al sistema */
  pr_info("my_pdrv_init: Procedo a agregar la estructura cdev al sistema\n");
  if (cdev_add(&state.chardev, state.devtype, MINOR_COUNT) != 0) 
  {
    pr_err(DEVICE_NAME": cdev_add: No se pude agregar el cdev\n");
    unregister_chrdev_region(state.devtype, MINOR_COUNT);
    return -EPERM;    // -1
  }
  pr_info("my_pdrv_init: Estructura cdev agregada al sistema\n");

  /* Voy a crear la clase */
  pr_info("my_pdrv_init: Voy a crear la clase \n");
  state.devclass = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
  if (IS_ERR(state.devclass)) 
  {
    pr_err(DEVICE_NAME": class_create: No se pudo crear la clase\n");
    unregister_chrdev_region(state.devtype, MINOR_COUNT);
    cdev_del(&state.chardev);
    return PTR_ERR(state.devclass);   // Handling null pointer cap2 Linux Drivers Development
  }
  pr_info("my_pdrv_init: Clase creada \n");

  /* Voy a crear el dispositivo */
  pr_info("my_pdrv_init: Voy a crear el dispositivo \n");
  state.dev = device_create(state.devclass, DEVICE_PARENT, state.devtype, NULL, DEVICE_NAME);
  if (IS_ERR(state.dev)) 
  {
    pr_err(DEVICE_NAME": device_create: No se pudo crear el dispositivo\n");
    unregister_chrdev_region(state.devtype, MINOR_COUNT);
    cdev_del(&state.chardev);
    class_destroy(state.devclass);  // Destruyo la clase creada en el paso previo
    return PTR_ERR(state.dev);
  }
  pr_info("my_pdrv_init: Dispositivo creado \n");

  pr_info("my_pdrv_init: Finalmente platform_driver_register\n");
  if(platform_driver_register(&mypdriver) < 0)
  {
    pr_err(DEVICE_NAME": device_create: No se pudo crear el dispositivo\n");
    unregister_chrdev_region(state.devtype, MINOR_COUNT);
    cdev_del(&state.chardev);
    class_destroy(state.devclass);          // Destruyo la clase creada en el paso previo
    device_destroy(state.devclass, state.devtype);  // Destruyo el device creado en el paso previo
    platform_driver_unregister(&mypdriver); 
    return PTR_ERR(state.dev);
  }

  pr_info("my_pdrv_init: Finalmente termine\n");
  return 0;
}
module_init(my_pdrv_init);

static void __exit my_pdrv_remove (void)    // se ejecuta cuando hago rmmod
{
  pr_info("my_pdrv_remove: Remuevo el dispositivo!\n");

  iounmap(state._reg_cm);
  iounmap(state._reg_i2c2);
  iounmap(state._reg_cm_per);
  free_irq(state.irq, NULL);

  pr_info("my_pdrv_remove: DesRegistro el driver!\n"); 
  platform_driver_unregister(&mypdriver);
  pr_info("my_pdrv_remove: Destruyo el dispositivo!\n");  
  device_destroy(state.devclass, state.devtype);  
  pr_info("my_pdrv_remove: Destruyo la clase!\n");
  class_destroy(state.devclass); 
  pr_info("my_pdrv_remove: Quito cdev del sistema!\n");
  cdev_del(&state.chardev);       
  pr_info("my_pdrv_remove: Desaloco cdev!\n");
  unregister_chrdev_region(state.devtype, MINOR_COUNT);
}
module_exit(my_pdrv_remove);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Fuschetto");
MODULE_DESCRIPTION("My platform Hello World module");





