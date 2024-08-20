class CommandDetection:
    def __init__(self):
        self.verbsID = {"خاموش": 0x00, "روشن": 0xFF, "کن": 0xFF, "نکن": 0x00}
        self.placeID = {"اتاق": 0x1, "آشپزخانه": 0x2, "پذیرایی": 0x3, "هال": 0x4, "بالکن": 0x5, "بالکنِ هال": 0x6, "بالکنِ نیما": 0x7, "بالکنِ آشپزخانه": 0x8}
        self.itemID = {"کولر": 0x1, "چراغ": 0x2, "لامپ": 0x3}
        self.dictionary = ["خاموش", "روشن", "کن", "نکن", "اتاق", "آشپزخانه", "پذیرایی", "هال", "بالکن", "بالکنِ هال", "بالکنِ نیما", "بالکنِ آشپزخانه", "کولر", "چراغ", "لامپ"]

    def proceedCode(self, textAudio):
        # Initialize a 24-bit binary code as a list of '0's
        code = ['0'] * 24

        # Split text into words
        text = textAudio.split()

        for txt in text:
            # Find the most similar word from the dictionary
            txt = self.findSimilarWord(txt, self.dictionary)
            if txt:
                # Update code based on matched word
                if txt in self.placeID:
                    code[8:16] = format(self.placeID[txt], '08b')  # Place ID (8 bits)
                if txt in self.itemID:
                    code[16:24] = format(self.itemID[txt], '08b')  # Item ID (8 bits)
                if txt in self.verbsID:
                    # Get the existing value in binary and convert to int
                    current_verbs_id = int(''.join(code[0:8]), 2)
                    # XOR with the new value
                    new_verbs_id = self.verbsID[txt]
                    xor_result = current_verbs_id ^ new_verbs_id
                    # Format the result back to 8 bits binary and update code
                    code[0:8] = format(xor_result, '08b')

        return ''.join(code)  # Convert list to string

    @staticmethod
    def levenshteinDistance(word1, word2):
        len1, len2 = len(word1), len(word2)
        dp = [[0] * (len2 + 1) for _ in range(len1 + 1)]

        for i in range(len1 + 1):
            dp[i][0] = i
        for j in range(len2 + 1):
            dp[0][j] = j

        for i in range(1, len1 + 1):
            for j in range(1, len2 + 1):
                if word1[i - 1] == word2[j - 1]:
                    dp[i][j] = dp[i - 1][j - 1]
                else:
                    dp[i][j] = 1 + min(dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1])

        return dp[len1][len2]

    def findSimilarWord(self, input_word, dictionary):
        min_distance = float('inf')
        most_similar_word = None

        for word in dictionary:
            distance = self.levenshteinDistance(input_word, word)
            if distance < min_distance:
                min_distance = distance
                most_similar_word = word

        return most_similar_word
