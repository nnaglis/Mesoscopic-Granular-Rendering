import pyexr
import numpy as np

# Find all images in the current directory
import os
import glob

# Get all the exr files in the images directory
exr_files = glob.glob("images/*.exr")

# do the following for each exr file
for exr_file in exr_files:
    print ("-------------------")
    print("Processing", exr_file)
    # Load the image
    image = pyexr.read(exr_file, "default")  # (1000,1000,4) np array

    #if alpha channel is not present, add it
    if image.shape[2] == 3:
        alpha_channel = np.ones((image.shape[0], image.shape[1], 1))
        image = np.concatenate((image, alpha_channel), axis=2)

    # print the shape of the image
    # print("Shape of the image is", image.shape)

    # Compute the Euclidean norm (L2 norm) of the RSE for each pixel
    l2_rse = np.sqrt(np.sum(np.square(image[:, :, :3]), axis=2))
    # print the max value of the L2 norm
    max_l2_rse = np.max(l2_rse)
    # print("Max value of the L2 norm is", max_l2_rse)

    # find a pixel that would have the largest value any single channel
    max_pixel = np.max(image[:, :, :3])
    # print("Max pixel value is", max_pixel)

    # calculate the mean of the image
    mean_value = np.mean(image[:, :, :3])
    # print("Mean value of the image is", mean_value)

    # calculate the standard deviation of the image
    std_value = np.std(image[:, :, :3])
    # print("Standard deviation of the image is", std_value)

    # cut the image to contain only the circle in the center
    # get the center of the image
    center = np.array(image.shape[:2]) / 2
    # print("Center of the image is", center)
    # get the radius of the circle
    radius = 476
    # create a mask for the circle
    mask = np.zeros(image.shape[:2])
    for i in range(image.shape[0]):
        for j in range(image.shape[1]):
            if np.linalg.norm([i, j] - center) < radius:
                mask[i, j] = 1

    # apply the mask to the image
    image = image * mask[:, :, np.newaxis]



    # print(image.shape)

    # Consider valid pixels to be the pixels that have an alpha value of 1 and are not fully black
    valid_pixels = np.where((image[:, :, 3] == 1) & (np.sum(image[:, :, :3], axis=2) != 0))
    # Save valid pixels to an exr file
    valid_pixels_image = np.zeros(image.shape)
    valid_pixels_image[valid_pixels] = image[valid_pixels]
    # pyexr.write("scene11_valid_pixels.exr", valid_pixels_image)



    # Find the max value of the image of combined RGB channels
    max_value = np.max(image[valid_pixels][:, :3])
    # print("Max value of the image is", max_value)

    # Find the min value of the image of combined RGB channels
    mean_value = np.mean(image[valid_pixels][:, :3])
    # print("Mean value of the image is", mean_value)


    std_value = np.std(image[valid_pixels][:, :3])
    # print("Standard deviation of the image is", std_value)



    filter_value = std_value + mean_value
    # print("Filter value is", filter_value)
    # filter out image values that are greater than std_value (set RGB to 0, but keep the alpha channel the same)
    image_filtered = np.copy(image)
    for i in range(image.shape[0]):
        for j in range(image.shape[1]):
                # print("___TEST___")
                # print(image[i, j, :3])
                # check and print which channel value is greater than filter_value
                # print("red",image[i, j, 0],image[i, j, 0] > filter_value)
                # print("green",image[i, j, 1],image[i, j, 1] > filter_value)
                # print("blue",image[i, j, 2],image[i, j, 2] > filter_value)
                # print("with .any",image[i, j, :3],(image[i, j, :3] > filter_value).any())
                
                # print("___TEST___")
            if (image[i, j, :3] > filter_value).any():
                image_filtered[i, j, :3] = 0


    #mean value of the filtered image
    mean_value = np.mean(image_filtered[valid_pixels][:, :3])
    print("MEAN VALUE OF THE FILTERED IMAGE", mean_value)

    #max value of the filtered image
    max_value = np.max(image_filtered[valid_pixels][:, :3])
    print("MAX VALUE OF THE FILTERED IMAGE", max_value)

    # Save the cut image 
    pyexr.write(exr_file + "_cut.exr", image) 

    # Save the image
    pyexr.write(exr_file + "_filtered.exr", image_filtered)


    # try otsu thresholding
    import cv2
    from scipy.ndimage import median_filter

    # apply Gaussian blur to the image
    # blur = cv2.GaussianBlur(image,(41,41),0)
    # apply meadian filter to the individual channels
    blur = np.zeros(image.shape)
    for i in range(3):
        blur[:, :, i] = median_filter(image[:, :, i], size=4)

    


    #save the blurred image
    # set the alpha channel to 1
    blur[:, :, 3] = 1
    # pyexr.write("scene11_blur.exr", blur)

    # calculate the max value of the image
    max_value = np.max(blur[valid_pixels][:, :3])
    # print("Max value of the image is", max_value)

    # convert the image to uint8
    image_uint8 = (blur[:, :, :3]/max_value*255).astype(np.uint8)


    # convert the image to grayscale
    gray = cv2.cvtColor(image_uint8, cv2.COLOR_RGB2GRAY)

    # save the grayscale image
    # cv2.imwrite("scene11_gray.png", gray)

    #print the shape of the grayscale image
    # print("Shape of the grayscale image is", gray.shape)

    # save the grayscale image with the circle cut out
    # cv2.imwrite("scene11_gray_cut.png", gray)


    # apply the circle mask to the grayscale image
    mask = mask.astype(bool)
    masked_gray = gray[mask]

    # save the grayscale image
    # cv2.imwrite("scene11_graymasked.png", masked_gray)

    # mask2 = masked_gray > 0
    # masked_gray = masked_gray[mask2]


    # apply otsu thresholding to find multiple thresholds
    ret,thresh1 = cv2.threshold(masked_gray,0,255,cv2.THRESH_BINARY+cv2.THRESH_OTSU)
    # print("Threshold value is", ret)
    # convert threshold value to float
    ret = ret/255*max_value
    # print("Threshold value (in 32bit) is", ret)
    # apply the threshold to the image
    image_filtered = np.copy(image)
    for i in range(image.shape[0]):
        for j in range(image.shape[1]):
            if (image[i, j, :3] > ret).any():
                image_filtered[i, j, :3] = 0

    #filter the grayscale image
    gray_filtered = np.copy(gray)
    gray_filtered[gray > 4.0] = 0

    # save the grayscale image
    # cv2.imwrite("scene11_grayfiltered.png", gray_filtered)

    print ("-------------------")
    # Save the image
    # pyexr.write(exr_file + "_filtered_otsu.exr", image_filtered)