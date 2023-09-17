/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Aaron Aprati"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
     struct aesd_dev *dev;
     
     mutex_lock(&aesd_device.lock); // Lock the mutex

    // Get the device structure from the inode's private data
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev; // Store the device structure in the file's private data

    mutex_unlock(&aesd_device.lock); // Unlock the mutex

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev = filp->private_data; // Get the device structure

    mutex_lock(&aesd_device.lock); // Lock the mutex

    PDEBUG("release");

    // Check if there's any temporary buffer that hasn't been written to the circular buffer
    if (dev->temp_buffer)
    {
        kfree(dev->temp_buffer); // Free the temporary buffer
        dev->temp_buffer = NULL; // Reset the pointer
    }

    // Optionally, you can also reset any other state or flags associated with this file descriptor

    mutex_unlock(&aesd_device.lock); // Unlock the mutex

    return 0; // Return 0 to indicate successful release
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data; // Get the device structure
    ssize_t retval = 0;
    size_t bytes_to_read;
    size_t entry_offset_byte;
    struct aesd_buffer_entry *entry;

    mutex_lock(&aesd_device.lock); // Lock the mutex

    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);

    // Find the buffer entry corresponding to the current file position
    entry = aesd_circular_buffer_find_entry_offset_for_fpos(
                &dev->circular_buffer, *f_pos, &entry_offset_byte);
    if (!entry)
    {
    	mutex_unlock(&aesd_device.lock); // Unlock the mutex
        // No more data to read
        return 0;
    }

    // Determine the number of bytes to read from this entry
    bytes_to_read = entry->size - entry_offset_byte;
    if (bytes_to_read > count)
    {
        bytes_to_read = count;
    }

    // Copy data from the device's buffer to the user's buffer
    if (copy_to_user(buf, entry->buffptr + entry_offset_byte, bytes_to_read))
    {
    	mutex_unlock(&aesd_device.lock); // Unlock the mutex
        // Failed to copy data to user space
        return -EFAULT;
    }

    // Update the file position and return value
    *f_pos += bytes_to_read;
    retval = bytes_to_read;

    mutex_unlock(&aesd_device.lock); // Unlock the mutex

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev = filp->private_data; // Get the device structure from the file's private data
    char *kern_buf; // Buffer to hold data in kernel space
    
    mutex_lock(&aesd_device.lock); // Lock the mutex
    
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
     // Allocate memory for the kernel buffer
    kern_buf = kmalloc(count, GFP_KERNEL);
    if (!kern_buf)
    
	mutex_unlock(&aesd_device.lock); // Unlock the mutex    
        return retval; // Return memory allocation error if kmalloc fails

    // Copy data from user space to kernel space
    if (copy_from_user(kern_buf, buf, count)) {
        retval = -EFAULT; // Set return value to indicate bad address error
        goto out_free; // Jump to the out_free label to free any allocated resources
    }

    // TODO: handle write

    // Check if the last character in the user buffer is a newline
    if (buf[count - 1] == '\n') {
        // If there's a previous unterminated write, concatenate it with the current write
        if (dev->temp_buffer) {
            size_t temp_size = strlen(dev->temp_buffer);
            char *combined_buf = kmalloc(temp_size + count, GFP_KERNEL);
            if (!combined_buf) {
                retval = -ENOMEM;
                kfree(kern_buf);
                mutex_unlock(&aesd_device.lock); // Unlock the mutex
                return retval;
            }
            strcpy(combined_buf, dev->temp_buffer);
            strcat(combined_buf, kern_buf);
            kfree(dev->temp_buffer);
            kern_buf = combined_buf;
            count = temp_size + count;
        }

        // Create a new entry for the circular buffer
        struct aesd_buffer_entry new_entry;
        new_entry.buffptr = kern_buf;
        new_entry.size = count;

        // Add the new entry to the circular buffer
        aesd_circular_buffer_add_entry(&dev->circular_buffer, &new_entry);

        // If the circular buffer is full, free the oldest entry
        if (dev->circular_buffer.full) {
            kfree(dev->circular_buffer.entry[dev->circular_buffer.out_offs].buffptr);
        }

        retval = count;
    } 
    else {
        // If the write is unterminated, save it for the next write operation
        dev->temp_buffer = kern_buf;
    }
    mutex_unlock(&aesd_device.lock); // Unlock the mutex
    return retval;
    
out_free:
    kfree(kern_buf); // Free the allocated kernel buffer
    mutex_unlock(&aesd_device.lock); // Unlock the mutex
    return retval; // Return the error value
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
     // Initialize the circular buffer
    aesd_circular_buffer_init(&aesd_device.circular_buffer);

    // Dynamically allocate memory for each buffer entry using kmalloc
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circular_buffer, index) {
        entry->buffptr = kmalloc(INITIAL_BUFFER_SIZE, GFP_KERNEL);
        if (!entry->buffptr) {
            printk(KERN_ERR "Failed to allocate memory for buffer entry %d\n", index);
            while (index--) {
                kfree(aesd_device.circular_buffer.entry[index].buffptr);
            }
            unregister_chrdev_region(dev, 1);
            return -ENOMEM;
        }
        entry->size = 0;
    }
    
    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    uint8_t index;
    struct aesd_buffer_entry *entry;

    // Delete the cdev structure
    cdev_del(&aesd_device.cdev);

    // Cleanup AESD specific portions
    // Free any memory associated with the circular buffer entries
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circular_buffer, index) {
        if (entry->buffptr) {
            kfree(entry->buffptr); // Free the memory associated with each buffer entry
        }
    }

    // Optionally, if there's a temporary buffer, free it
    if (aesd_device.temp_buffer) {
        kfree(aesd_device.temp_buffer);
        aesd_device.temp_buffer = NULL;
    }

    // Optionally, destroy any locks or synchronization primitives
    mutex_destroy(&aesd_device.lock);

    // Unregister the character device region
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
