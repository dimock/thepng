% solve test task

function do_process(varargin)
    thrCoef = 1.1;
    in_name = 'problem_2.png';
    out_name = 'my_result.png';
    img = imread(in_name);
    img_gray = rgb2gray(img);
    %figure(1), imshow(img_gray);
    img_dbl = im2double(img_gray);
    img_dbl_comp = imcomplement(img_dbl);
    %[counts, x] = imhist(img_dbl);
    %figure(2), stem(x, counts);
    Thr = graythresh(img_dbl)*thrCoef;
    bw_mask = im2bw(img_dbl, Thr);
    bw_mask = im2bw(img_dbl_comp, 0.05) .* bw_mask;
    % figure(3), imshow(bw_mask);
    %figure(4), imshow(bw_mask_inv);
    img_back = bw_mask .* img_dbl;
    S = 0;
    num = 0;
    W=size(img_back, 2);
    H=size(img_back, 1);
    x_rect = 0;
    y_ellipse_start = H;
    y_ellipse_stop = 0;
    xleft = H;
    xright = 0;
    segment_n = 0;
    for y=1:H
        % find horizontal line
        if y_ellipse_start == H
            p = y_line_prod( y, img_back);
            if segment_n == 0 && p == 0
                segment_n = 1;
            elseif segment_n == 1 && p ~= 0
                y_ellipse_start = y;
                segment_n = 2;
            end
        elseif y_ellipse_stop == 0 && segment_n > 1
            p = y_line_prod( y, img_back);
            if p == 0 && segment_n == 2
                segment_n = 3;
            end
            if p ~= 0 && segment_n == 3
                y_ellipse_stop = y;
            end
        end
        for x=1:W
            v = img_back(y, x);
            if y > y_ellipse_start && v == 0
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
    ellipse_mask = ones(size(img_back));
    xcenter_ellipse = (xleft + xright) / 2;
    for y=y_ellipse_start:y_ellipse_stop
        for x=1:W
            v = bw_mask(y, x);
            if v == 0 && x < xcenter_ellipse
                x1 = 2*xcenter_ellipse - x;
                r = bw_mask(y, x1);
                bw_mask(y, x1) = 0;
                q = bw_mask(y, x1);
            end
        end
        ellipse_mask(y,:) = bw_mask(y,:);
    end
    M = S / num;
    img_denoise = (ones(size(img_back)) * M) .* bw_mask;
    bw_mask_inv = imcomplement(bw_mask);
    img_figures = img_dbl .* bw_mask_inv;
    img_result = img_denoise + img_figures;
    figure(5), imshow(img_result);
    figure(3), imshow(ellipse_mask);
    imwrite(img_result, out_name, 'png');
end

function p = y_line_prod(y, mask)
    y_line_mask = mask(y, :);
    p = prod(y_line_mask);
end
