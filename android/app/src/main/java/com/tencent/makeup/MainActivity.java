// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2017 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

package com.tencent.makeup;
import android.annotation.SuppressLint;
import android.content.Context;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.MediaController;
import android.widget.VideoView;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.concurrent.TimeUnit;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class MainActivity extends Activity
{
    private ImageView imageView;
    private VideoView videoView;
    private FrameLayout videoContainer;
    private EditText positivePromptText;
    private EditText negativePromptText;
    private EditText stepText;
    private EditText seedText;
    private String mp4Url;
    final static String defaultPositivePrompt = "apple on the table, realistic, highly detailed, vibrant colors, natural lighting, shiny surface, wooden table, shadows, high quality, photorealistic, masterpiece";
    final static String defaultNegativePrompt = "blurry, deformed, bad anatomy, disfigured, poorly drawn, mutation, mutated, extra limb, ugly, missing limb, blurry, floating, disconnected, malformed, blur, out of focus, bad lighting, unrealistic, text";
    private Bitmap showBitmap;
    private static final int MAX_RETRIES = 20;
    private static final long RETRY_DELAY_MS = 5000;

    private StableDiffusion sd = new StableDiffusion();

    interface VideoFetchCallback {
        void onVideoLinkFetched(String videoUrl);
    }

    /** Called when the activity is first created. */
    @SuppressLint("MissingInflatedId")
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        String path = MainActivity.this.getFilesDir().getAbsolutePath();
        String name = "vocab.txt";
        copy(MainActivity.this, name, path, name);
        String file = path+File.separator+name;

        String name1 = "log_sigmas.bin";
        copy(MainActivity.this, name1, path, name1);
        String file1 = path+File.separator+name1;

        boolean ret_init = sd.Init(getAssets(), file, file1);
        if (!ret_init)
        {
            Log.e("MainActivity", "SD Init failed");
        }

        imageView = (ImageView) findViewById(R.id.resView);
        positivePromptText = (EditText) findViewById(R.id.pos);
        negativePromptText = (EditText) findViewById(R.id.neg);

        stepText = (EditText) findViewById(R.id.step);
        seedText = (EditText) findViewById(R.id.seed);


        showBitmap = Bitmap.createBitmap(256,256,Bitmap.Config.ARGB_8888);

        Button buttonDefaultPositive = (Button) findViewById(R.id.defaultPositive);
        buttonDefaultPositive.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                positivePromptText.setText(MainActivity.defaultPositivePrompt);
            }
        });

        Button buttonDefaultNegative = (Button) findViewById(R.id.defaultNegative);
        buttonDefaultNegative.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                negativePromptText.setText(MainActivity.defaultNegativePrompt);
            }
        });

        Button buttonInitImage = (Button) findViewById(R.id.init);
        buttonInitImage.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                videoView.setVisibility(View.GONE);
                imageView.setVisibility(View.VISIBLE);
                Intent i = new Intent(Intent.ACTION_PICK);
                i.setType("image/*");
                startActivityForResult(i, 1);
            }
        });

        Button buttonTXT2IMG = (Button) findViewById(R.id.txt2img);
        buttonTXT2IMG.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                videoView.setVisibility(View.GONE);
                imageView.setVisibility(View.VISIBLE);

                getWindow().setFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE, WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
                new Thread(new Runnable() {
                    public void run() {

                        String positivePrompt = positivePromptText.getText().toString();
                        String negativePrompt = negativePromptText.getText().toString();
                        if (positivePrompt.isEmpty() || negativePrompt.isEmpty()) {
                            return;
                        }
                        int step = Integer.valueOf(stepText.getText().toString());
                        int seed = Integer.valueOf(seedText.getText().toString());

                        sd.txt2imgProcess(showBitmap,step,seed,positivePrompt,negativePrompt);

                        final Bitmap styledImage = showBitmap.copy(Bitmap.Config.ARGB_8888,true);
                        imageView.post(new Runnable() {
                            public void run() {
                                imageView.setImageBitmap(styledImage);
                                getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
                            }
                        });
                    }
                }).start();
            }
        });

        Button buttonIMG2IMG = (Button) findViewById(R.id.img2img);
        buttonIMG2IMG.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                videoView.setVisibility(View.GONE);
                imageView.setVisibility(View.VISIBLE);
                getWindow().setFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE, WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
                new Thread(new Runnable() {
                    public void run() {

                        String positivePrompt = positivePromptText.getText().toString();
                        String negativePrompt = negativePromptText.getText().toString();

                        if (positivePrompt.isEmpty() || negativePrompt.isEmpty()) {
                            return;
                        }

                        int step = Integer.valueOf(stepText.getText().toString());
                        int seed = Integer.valueOf(seedText.getText().toString());

                        sd.img2imgProcess(showBitmap,step,seed,positivePrompt,negativePrompt);

                        final Bitmap styledImage = showBitmap.copy(Bitmap.Config.ARGB_8888,true);
                        imageView.post(new Runnable() {
                            public void run() {
                                imageView.setImageBitmap(styledImage);
                                getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
                            }
                        });
                    }
                }).start();
            }
        });
        videoContainer = (FrameLayout) findViewById(R.id.frame_layout);

        imageView = (ImageView) findViewById(R.id.resView);
        videoView = (VideoView) findViewById(R.id.videoView);
        videoView.setVisibility(View.GONE);
        imageView.setVisibility(View.GONE);

        Button videoButton = (Button) findViewById(R.id.txt2video);
        videoButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                videoView.setVisibility(View.VISIBLE);
                imageView.setVisibility(View.GONE);
                fetchVideoLink(new VideoFetchCallback() {
                    @Override
                    public void onVideoLinkFetched(String videoUrl) {
                        // Start polling the video URL for availability
                        pollVideoAvailability(videoUrl, videoView);
                    }
                });
            }
        });
    }

    // Fetch the video link from the API
    private void fetchVideoLink(VideoFetchCallback callback) {
//        OkHttpClient client = new OkHttpClient();
//        Request request = new Request.Builder()
//                .url("https://pub-3626123a908346a7a8be8d9295f44e26.r2.dev/video_generations/b34f8014-ff42-4efa-85ea-3f8815bb6a28.mp4") // Replace with your actual API URL
//                .build();
//
//        client.newCall(request).enqueue(new Callback() {
//            @Override
//            public void onFailure(Call call, IOException e) {
//                e.printStackTrace();
//            }
//
//            @Override
//            public void onResponse(Call call, Response response) throws IOException {
//                try {
//                    if (response.isSuccessful()) {
////                        String videoUrl = response.body().string(); // Assuming the response contains the URL as plain text
//                        String videoUrl = "https://pub-3626123a908346a7a8be8d9295f44e26.r2.dev/video_generations/b34f8014-ff42-4efa-85ea-3f8815bb6a28.mp4";
//                        runOnUiThread(() -> callback.onVideoLinkFetched(videoUrl));
//                    }
//                } finally {
//                    // Always close the response to avoid leaks
//                    response.close();
//                }
//            }
//        });

        OkHttpClient client = new OkHttpClient.Builder()
                .connectTimeout(120, TimeUnit.SECONDS) // Set the connection timeout
                .readTimeout(300, TimeUnit.SECONDS)    // Set the read timeout
                .writeTimeout(300, TimeUnit.SECONDS)   // Set the write timeout
                .build();

        // Create the request body in JSON format
        JSONObject jsonBody = new JSONObject();
        try {
            jsonBody.put("model_id", "zeroscope");
            jsonBody.put("prompt", this.positivePromptText.getText().toString());
            jsonBody.put("negative_prompt", this.negativePromptText.getText().toString());
            jsonBody.put("height", 320);
            jsonBody.put("width", 512);
            jsonBody.put("num_frames", 16);
            jsonBody.put("num_inference_steps", 20);
            jsonBody.put("guidance_scale", 7);
            jsonBody.put("upscale_height", 640);
            jsonBody.put("upscale_width", 1024);
            jsonBody.put("upscale_strength", 0.6);
            jsonBody.put("upscale_guidance_scale", 12);
            jsonBody.put("upscale_num_inference_steps", 20);
            jsonBody.put("output_type", "mp4");
            jsonBody.put("webhook", JSONObject.NULL);
            jsonBody.put("track_id", JSONObject.NULL);
        } catch (Exception e) {
            e.printStackTrace();
        }

        // Set the media type to JSON
        MediaType JSON = MediaType.parse("application/json; charset=utf-8");
        RequestBody requestBody = RequestBody.create(JSON, jsonBody.toString());

        // Create the POST request
        Request request = new Request.Builder()
                .url("https://modelslab.com/api/v6/video/text2video") // Replace with your actual POST API URL
                .post(requestBody)
                .build();

        // Execute the request
        client.newCall(request).enqueue(new Callback() {
            @Override
            public void onFailure(Call call, IOException e) {
                e.printStackTrace();
            }

            @Override
            public void onResponse(Call call, Response response) throws IOException {
                try {
                    if (response.isSuccessful()) {
                        // Parse the response as a JSONObject
                        String responseBody = response.body().string();
                        System.out.println(responseBody);
                        JSONObject jsonResponse = new JSONObject(responseBody);

                        // Extract the first link from the 'output' array
                        JSONArray outputArray = jsonResponse.getJSONArray("future_links");
                        String videoUrl = outputArray.getString(0); // Get the first link
                        System.out.println(videoUrl);
                        // Pass the video URL back to the UI thread
                        runOnUiThread(() -> callback.onVideoLinkFetched(videoUrl));
                    } else {
                        // Handle unsuccessful responses if needed
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                } finally {
                    response.close();
                }
            }
        });
    }

    private void pollVideoAvailability(String videoUrl, VideoView videoView) {
        OkHttpClient client = new OkHttpClient();

        new Thread(new Runnable() {
            @Override
            public void run() {
                int attempts = 0;

                while (attempts < MAX_RETRIES) {
                    Request request = new Request.Builder()
                            .url(videoUrl) // The video URL from the API
                            .build();

                    try {
                        Response response = client.newCall(request).execute();

                        // If the video link is ready (i.e., not 404), load the video
                        if (response.isSuccessful()) {
                            runOnUiThread(() -> {
                                // Load and play the video
                                Uri uri = Uri.parse(videoUrl);
                                videoView.setVideoURI(uri);
                                videoView.setMediaController(new MediaController(MainActivity.this));
                                videoView.start();

                            });
                            break; // Exit the loop once the video is ready
                        } else {
                            // If it's a 404, the video is not ready, retry after delay
                            Log.e("MainActivity", "Error: " + response.code());
                            attempts++;
                            Thread.sleep(RETRY_DELAY_MS); // Wait before retrying
                        }

                    } catch (IOException | InterruptedException e) {
                        e.printStackTrace();
                        break;
                    }
                }
            }
        }).start();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode) {
            case 1:
                if (resultCode == RESULT_OK && null != data) {
                    Uri selectedImage = data.getData();
                    try {
                        final Bitmap tmp = decodeUri(selectedImage);
                        showBitmap = Bitmap.createScaledBitmap(tmp,256,256,false);
                        imageView.setImageBitmap(showBitmap);
                    } catch (FileNotFoundException e) {
                        throw new RuntimeException(e);
                    }
                }
                break;
            default:
                break;
        }
    }

    private void copy(Context myContext, String ASSETS_NAME, String savePath, String saveName) {
        String filename = savePath + "/" + saveName;
        File dir = new File(savePath);
        if (!dir.exists())
            dir.mkdir();
        try {
            if (!(new File(filename)).exists()) {
                InputStream is = myContext.getResources().getAssets().open(ASSETS_NAME);
                FileOutputStream fos = new FileOutputStream(filename);
                byte[] buffer = new byte[7168];
                int count = 0;
                while ((count = is.read(buffer)) > 0) {
                    fos.write(buffer, 0, count);
                }
                fos.close();
                is.close();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private Bitmap decodeUri(Uri selectedImage) throws FileNotFoundException
    {
        // Decode image size
        BitmapFactory.Options o = new BitmapFactory.Options();
        o.inJustDecodeBounds = true;
        BitmapFactory.decodeStream(getContentResolver().openInputStream(selectedImage), null, o);

        // The new size we want to scale to
        final int REQUIRED_SIZE = 256;

        // Find the correct scale value. It should be the power of 2.
        int width_tmp = o.outWidth, height_tmp = o.outHeight;
        int scale = 1;
        while (true) {
            if (width_tmp / 2 < REQUIRED_SIZE
                    || height_tmp / 2 < REQUIRED_SIZE) {
                break;
            }
            width_tmp /= 2;
            height_tmp /= 2;
            scale *= 2;
        }

        // Decode with inSampleSize
        BitmapFactory.Options o2 = new BitmapFactory.Options();
        o2.inSampleSize = scale;
        Bitmap bitmap = BitmapFactory.decodeStream(getContentResolver().openInputStream(selectedImage), null, o2);

        // Rotate according to EXIF
        int rotate = 0;
        try
        {
            ExifInterface exif = new ExifInterface(getContentResolver().openInputStream(selectedImage));
            int orientation = exif.getAttributeInt(ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_NORMAL);
            switch (orientation) {
                case ExifInterface.ORIENTATION_ROTATE_270:
                    rotate = 270;
                    break;
                case ExifInterface.ORIENTATION_ROTATE_180:
                    rotate = 180;
                    break;
                case ExifInterface.ORIENTATION_ROTATE_90:
                    rotate = 90;
                    break;
            }
        }
        catch (IOException e)
        {
            Log.e("MainActivity", "ExifInterface IOException");
        }

        Matrix matrix = new Matrix();
        matrix.postRotate(rotate);
        return Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
    }
}
