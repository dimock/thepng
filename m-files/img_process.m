% solve test task
thrCoef = 1.1;
in_name = 'problem_2.png';
out_name = 'my_result.png';
img = imread(in_name);
img_gray = rgb2gray(img);
%figure(1), imshow(img_gray);
img_dbl = im2double(img_gray);
img_dbl_comp = imcomplement(img_dbl);
[counts, x] = imhist(img_dbl);
%figure(2), stem(x, counts);
Thr = graythresh(img_dbl)*thrCoef;
bw_mask = im2bw(img_dbl, Thr);
bw_mask = im2bw(img_dbl_comp, 0.05) .* bw_mask;
%figure(3), imshow(bw_mask);
%figure(4), imshow(bw_mask_inv);
img_back = bw_mask .* img_dbl;
S = 0;
num = 0;
W=size(img_back, 2);
H=size(img_back, 1);
H1 = 2*H/3;
xleft = H;
xright = 0;
for y=1:H
    for x=1:W
        v = img_back(y, x);
        if y > H1 && v == 0
            if x < xleft
                xleft = x;
            elseif x > xright
                xright = x;
            end
        end
        if v == 0
            continue
        end
        S = S + v;
        num = num + 1;
    end
end
xcenter = (xleft + xright) / 2;
for y=1:H
    for x=1:W
        v = img_back(y, x);
        if y > H1 && v == 0 && x < xcenter
            x1 = 2*xcenter - x;
            bw_mask(y, x1) = 0;
        end
    end
end
M = S / num;
img_denoise = (ones(size(img_back)) * M) .* bw_mask;
bw_mask_inv = imcomplement(bw_mask);
img_figures = img_dbl .* bw_mask_inv;
img_result = img_denoise + img_figures;
figure(5), imshow(img_result);
imwrite(img_result, out_name, 'png');

