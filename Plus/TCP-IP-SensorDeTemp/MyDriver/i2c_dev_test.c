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

#define BMP280_SLAVE_DIRR_TX 0x76
#define REG_ID 0xD0

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

#define I2C_2_IRQSTATUS_XRDY (1 << 2)
#define I2C_2_IRQSTATUS_ARDY (1 << 3)
#define I2C_2_IRQSTATUS_RRDY (1 << 4)

#define I2C_2_CON_RESET (0 << 15)
#define I2C_2_SETFREQ 100000

/*************************************************************************************************/
/*************************************************************************************************/

/*************************************************************************************************/
/************************************* Estructura state ******************************************/
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
  pr_info(DEVICE_NAME": Lei:  %#x\n", old_value);
  old_value = old_value & ~(mask);  // Borro los datos que voy a escribir
  value = value & mask;   // Me quedo con los valores que van
  value = value | old_value;
  pr_info(DEVICE_NAME": Escribo el registro con:  %#x\n", value);
  iowrite32(value, base + offset);
  return;
}
/*************************************************************************************************/
/*************************************************************************************************/


/*************************************************************************************************/
/**
  @brief Funcion para setear registros

  @param reg
  @param offset

  @returns void
**/
/*************************************************************************************************/
uint32_t get_register (uint32_t *reg, uint32_t offset)
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
  @brief 

  @param void

  @returns void
**/
/*************************************************************************************************/
struct bmp280_dev
{
  uint8_t initialized;
  unsigned long page;
};

struct bmp280_dev bmp280_dev = {0, 0};

static DECLARE_WAIT_QUEUE_HEAD(my_wq);
static int condition = 0;

static int bmp280_init(void)
{

  return 0;
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
  uint32_t irqstate = ioread32 (state._reg_i2c2 + I2C_2_IRQSTATUS);  
  iowrite32(irqstate, state._reg_i2c2 + I2C_2_IRQSTATUS);

  pr_info(DEVICE_NAME": ISR: Entre al manejador de interrupciones irqstate es: %d\n", irqstate);

  if (( (irqstate & I2C_2_IRQSTATUS_XRDY)>> 2) == 0x1 ) 
  {
    pr_info(DEVICE_NAME": ISR: XRDY interrup\n");
    return IRQ_HANDLED;
  }

  if ( ((irqstate & I2C_2_IRQSTATUS_RRDY)  >> 4) == 0x1)
  {
    pr_info(DEVICE_NAME": ISR: RRDY interrup\n");
    return IRQ_HANDLED;
  }

  if ( ((irqstate & I2C_2_IRQSTATUS_ARDY) >> 3) == 0x1)
  {
    pr_info(DEVICE_NAME": ISR: ARDY interrup\n");
    return IRQ_HANDLED;
  }

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
  uint32_t aux=0, count=0;

  pr_info(DEVICE_NAME": Hola! Entre a open! \n");

  if (min < 0)
  {
    pr_err (DEVICE_NAME": Device not found\n");
    return -ENODEV; /* No such device */
  }

  /* Seteo los pines de la BBB */
  pr_info(DEVICE_NAME": Configuro los pines para I2C_2 \n");
  iowrite32(0x33, (state._reg_cm + (uint32_t) CONTROL_MODULE_UART1_CTSN) );  
	iowrite32(0x33, (state._reg_cm + (uint32_t) CONTROL_MODULE_UART1_RTSN) );

  /* Inicializo algunos registros del i2c */
  pr_info(DEVICE_NAME": Inicializo algunos registros del i2c \n");
  
  // Soft Reset
  pr_info(DEVICE_NAME": Realizo un reset \n");
  set_registers(state._reg_i2c2, I2C_2_SYSC, (uint32_t)0x0 , (uint32_t)0x0); 

  while ( (get_register(state._reg_i2c2, I2C_2_SYSC) >> 1) != 0 )  // Espero SoftReset
  {
    pr_info(DEVICE_NAME": Esperando el reset...\n");
    mdelay(1);
  }
  pr_info(DEVICE_NAME": Reset completado \n");

	iowrite32(3, state._reg_i2c2 + I2C_2_PSC); // ICLK = 12MHz
	// Duty High y Duty Low:
	iowrite32(53, state._reg_i2c2 + I2C_2_SCLH);  
	iowrite32(55, state._reg_i2c2 + I2C_2_SCLL); 
  iowrite32((I2C_2_IRQSTATUS_ARDY | I2C_2_IRQSTATUS_RRDY), state._reg_i2c2 + I2C_2_IRQENABLE_SET); // Habilitacion de interrupciones
  iowrite32(0, state._reg_i2c2 + I2C_2_SYSC);

	iowrite32((1 << 10) | (1 << 15), state._reg_i2c2 + I2C_2_CON);// Coloco en MASTER y ENABLE el I2C_2:

  pr_info(DEVICE_NAME": Registros del i2c inicializados \n");

  pr_info(DEVICE_NAME": Consigo una pagina para el buffer de recepcion\n");
  if((bmp280_dev.page=__get_free_page(GFP_KERNEL)) == 0)
  {
    pr_err (DEVICE_NAME": Insuficiente memoria\n");
    return -ENODEV; /* No such device */
  }

  pr_info(DEVICE_NAME": Inicializo el modulo\n");

  iowrite32((uint32_t)0x76, state._reg_i2c2 + I2C_2_SA);

  iowrite32(1, state._reg_i2c2 + I2C_2_CNT);

  pr_info(DEVICE_NAME": Espero que el bus este liberado\n");
	aux = ioread32(state._reg_i2c2 + I2C_2_IRQSTATUS_RAW);
	count= 0;
	while(aux & (1 << 12))
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

  set_registers(state._reg_i2c2, I2C_2_CON, (uint32_t)0x00008603, (uint32_t)0x00008603);

  pr_info(DEVICE_NAME": Duermo!\n");
  wait_event_interruptible(my_wq, condition != 0);

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
static ssize_t driver_read(struct file *file, char __user *buff, size_t size, loff_t *offset) 
{
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
  .release = driver_release
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
  //if ((state.irq = platform_get_irq(pdev, 0)) < 0)
  //{
  //  pr_err(DEVICE_NAME": platform_get_irq: No se pudo registrar el ISR\n");
  //  unregister_chrdev_region(state.devtype, MINOR_COUNT);
  //  cdev_del(&state.chardev);
  //  class_destroy(state.devclass);          // Destruyo la clase creada en el paso previo
  //  device_destroy(state.devclass, state.devtype);  // Destruyo el device creado en el paso previo
  //  platform_driver_unregister(&mypdriver);
  //  return -EPERM;    // -1
  //}
  //

  /* Implantamos el ISR */
  pr_info("%s: Implantamos el ISR \n", pdev->name);
  pr_info("%s: Numero del IRQ del device: %d\n", pdev->name, state.irq);

  state.irq = irq_of_parse_and_map((pdev->dev).of_node, 0);
  if (state.irq == 0)
  {
    pr_err(DEVICE_NAME": request_irq: No se pudo registrar el ISR   wtf men\n");
    return -EPERM;    // -1
  }


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
  
//  /* En vez de pedir memoria con ioremap() voy a usar */  // Consultar
//
//  /* Pedimos memoria fisica (ioremap, paginacion) platform_get_resource y devm_ioremap_resource */
//  /* Paper: Understanding the Genetic Makeup of Linux Device Drivers pg.3 */
//
//  pr_info("%s: Pido memoria fisica \n", pdev->name);
//  if ((memory = platform_get_resource(pdev, IORESOURCE_MEM, 0)) == 0)
//  {
//    pr_err(DEVICE_NAME": platform_get_resource : Recurso no disponible\n");
//    class_destroy(state.devclass);        // Destruyo la clase creada en el paso previo
//    device_destroy(state.devclass, state.devtype);  // Destruyo el device creado en el paso previo
//    unregister_chrdev_region(state.devtype, MINOR_COUNT);
//    return -EPERM;    // -1
//  }
//  pr_info("%s: Memoria fisica, recurso disponible.\n", pdev->name);
//  pr_info("%s: Nombre: %s ; Comienzo: %d ; Fin: %d\n", pdev->name, memory->name, memory->start, memory->end);
//
//  /* Aqui el driver tomara posecion de la region de memoria y la mapeara en el espacio virtual */
//  if(IS_ERR(state.base = devm_ioremap_resource(&pdev->dev, memory)))      // Podria haber usado ioremap tambien ioremap(&pdev->dev, mem->start, resource_size(mem)); ...
//  {
//    pr_err(DEVICE_NAME": devm_ioremap_resource : No se pude mamear en el VMA la memoria\n");
//    class_destroy(state.devclass);        // Destruyo la clase creada en el paso previo
//    device_destroy(state.devclass, state.devtype);  // Destruyo el device creado en el paso previo
//    unregister_chrdev_region(state.devtype, MINOR_COUNT);
//    return PTR_ERR(state.base);
//  }
  
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
  pr_info("Hasta la vista babe!\n");
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

static void __exit my_pdrv_remove (void)
{
  pr_info("my_pdrv_remove: Hasta luego!\n");
  platform_driver_unregister(&mypdriver);
}
module_exit(my_pdrv_remove);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Fuschetto");
MODULE_DESCRIPTION("My platform Hello World module");





